package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

const fileName = "Thornmarked.n64"

var wsdir string

// runGit runs git and returns standard output.
func runGit(args ...string) ([]byte, error) {
	var out bytes.Buffer
	cmd := exec.Command("git", args...)
	cmd.Stdout = &out
	cmd.Stderr = os.Stderr
	cmd.Dir = wsdir
	if err := cmd.Run(); err != nil {
		return nil, err
	}
	return out.Bytes(), nil
}

// getIsDirty returns true if the repository is dirty (has uncommitted changes).
func getIsDirty() (bool, error) {
	b, err := runGit("status", "--porcelain")
	if err != nil {
		return false, fmt.Errorf("could not check if repository is dirty: %w", err)
	}
	return len(b) != 0, nil
}

// getRevNum gets the revision number (the number of commits).
func getRevNum() (uint64, error) {
	b, err := runGit("rev-list", "--count", "--first-parent", "HEAD")
	if err != nil {
		return 0, fmt.Errorf("could not get revision number: %w", err)
	}
	s := strings.TrimSpace(string(b))
	n, err := strconv.ParseUint(s, 10, 64)
	if err != nil {
		return 0, fmt.Errorf("invalid revision number: %w", err)
	}
	return n, nil
}

func getCommitHash() (string, error) {
	b, err := runGit("rev-parse", "HEAD")
	if err != nil {
		return "", fmt.Errorf("could not get commit hash: %w", err)
	}
	return string(bytes.TrimSpace(b)), nil
}

// splitLine splits text at the first line break.
func splitLine(b []byte) (line, rest []byte) {
	i := bytes.IndexByte(b, '\n')
	if i == -1 {
		return b, nil
	}
	return b[:i], b[i+1:]
}

type commitInfo struct {
	Revision  uint64    `json:"revision"`
	Hash      string    `json:"hash"`
	Timestamp time.Time `json:"timestamp"`
	Message   string    `json:"message"`
}

// getCommitTimestamp gets the commit timestamp of the current commit.
func getCommitInfo() (*commitInfo, error) {
	revnum, err := getRevNum()
	if err != nil {
		return nil, err
	}

	chash, err := getCommitHash()
	if err != nil {
		return nil, err
	}

	b, err := runGit("cat-file", "commit", "HEAD")
	if err != nil {
		return nil, fmt.Errorf("could not get commit info: %w", err)
	}
	headers := make(map[string][]byte)
	for len(b) != 0 {
		var line []byte
		line, b = splitLine(b)
		if len(line) == 0 {
			break
		}
		var name, value []byte
		if i := bytes.IndexByte(line, ' '); i != -1 {
			name = line[:i]
			value = line[i+1:]
		} else {
			name = line
		}
		headers[string(name)] = value
	}
	msg := string(b)
	committer := headers["committer"]
	if committer == nil {
		return nil, errors.New("could not find committer")
	}
	cfields := bytes.Fields(committer)
	if len(cfields) < 2 {
		return nil, errors.New("could not parse committer")
	}
	ststamp := cfields[len(cfields)-2]
	stoff := cfields[len(cfields)-1]
	tstamp, err := strconv.ParseInt(string(ststamp), 10, 64)
	if err != nil {
		return nil, fmt.Errorf("invalid commit timestamp %q: %w", ststamp, err)
	}
	toff, err := strconv.ParseInt(string(stoff), 10, 64)
	if err != nil {
		return nil, fmt.Errorf("invalid time zone offset %q: %w", stoff, err)
	}
	toffh := toff / 100
	toffm := toff % 100
	loc := time.FixedZone("", int(toffh*60*60+toffm*60))
	ctime := time.Unix(tstamp, 0).In(loc)

	return &commitInfo{
		Revision:  revnum,
		Hash:      chash,
		Timestamp: ctime,
		Message:   msg,
	}, nil
}

type buildInfo struct {
	commitInfo
	BuildTimestamp time.Time `json:"buildTimestamp"`
}

// buildArtifact builds the desired artifact.
func buildArtifact() error {
	fmt.Fprintln(os.Stderr, "Building...")
	cmd := exec.Command("bazel", "build", "-c", "opt", "--cpu=n64", "//game")
	cmd.Stderr = os.Stderr
	cmd.Dir = wsdir
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("build failed: %w", err)
	}
	return nil
}

func createPackage(info *buildInfo, filename string) error {
	ofp, err := ioutil.TempFile(filepath.Dir(filename), "temp.*.gz")
	if err != nil {
		return err
	}
	defer func() {
		ofp.Close()
		os.Remove(ofp.Name())
	}()
	zfp, err := gzip.NewWriterLevel(ofp, gzip.BestCompression)
	if err != nil {
		return err
	}
	tw := tar.NewWriter(zfp)

	dinfo, err := json.Marshal(info)
	if err != nil {
		return err
	}
	if err := tw.WriteHeader(&tar.Header{
		Name:    "INFO.json",
		Size:    int64(len(dinfo)),
		Mode:    0444,
		ModTime: info.BuildTimestamp,
	}); err != nil {
		return err
	}
	if _, err := tw.Write(dinfo); err != nil {
		return err
	}

	inpath := filepath.Join(wsdir, "bazel-bin/game", fileName)
	fp, err := os.Open(inpath)
	if err != nil {
		return err
	}
	defer fp.Close()
	st, err := fp.Stat()
	if err != nil {
		return err
	}
	if err := tw.WriteHeader(&tar.Header{
		Name:    fileName,
		Size:    st.Size(),
		Mode:    0444,
		ModTime: info.BuildTimestamp,
	}); err != nil {
		return err
	}
	if _, err := io.Copy(tw, fp); err != nil {
		return err
	}

	if err := tw.Close(); err != nil {
		return err
	}
	if err := zfp.Close(); err != nil {
		return err
	}
	if err := ofp.Close(); err != nil {
		return err
	}
	if err := os.Rename(ofp.Name(), filename); err != nil {
		return err
	}
	return nil
}

func mainE() error {
	allowDirty := flag.Bool("allow-dirty", false, "allow upload with a dirty repository")
	flag.Parse()
	if args := flag.Args(); len(args) != 0 {
		return fmt.Errorf("unexpected argument: %q", args[0])
	}
	wsdir = os.Getenv("BUILD_WORKSPACE_DIRECTORY")
	if wsdir == "" {
		return errors.New("run this command from Bazel")
	}
	if !*allowDirty {
		dirty, err := getIsDirty()
		if err != nil {
			return err
		}
		if dirty {
			return errors.New("repository is dirty")
		}
	}
	cinfo, err := getCommitInfo()
	if err != nil {
		return err
	}
	if err := buildArtifact(); err != nil {
		return err
	}
	home := os.Getenv("HOME")
	if home == "" {
		return errors.New("$HOME not set")
	}
	now := time.Now()
	dest := filepath.Join(home, "Documents", "Artifacts",
		fmt.Sprintf("%s.r%04d.%s.gz", fileName, cinfo.Revision,
			now.UTC().Format("20060102150405")))
	if err := os.MkdirAll(filepath.Dir(dest), 0777); err != nil {
		return err
	}
	binfo := buildInfo{commitInfo: *cinfo, BuildTimestamp: now}
	if err := createPackage(&binfo, dest); err != nil {
		return err
	}
	return nil
}

func main() {
	if err := mainE(); err != nil {
		fmt.Fprintln(os.Stderr, "Error:", err)
		os.Exit(1)
	}
}
