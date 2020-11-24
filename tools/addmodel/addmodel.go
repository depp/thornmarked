package main

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"thornmarked/tools/getpath"
)

const (
	modelsPath    = "assets/models"
	modelsPackage = "//" + modelsPath
	identPrefix   = "MODEL_"
)

type label struct {
	pkg    string
	target string
}

func filterPackage(pkg string, ls []label) []string {
	var r []string
	for _, l := range ls {
		if l.pkg == pkg {
			r = append(r, l.target)
		}
	}
	return r
}

func splitLine(data []byte) (line, rest []byte) {
	i := bytes.IndexByte(data, '\n')
	if i < 0 {
		return data, nil
	}
	return data[:i], data[i+1:]
}

func bazelQuery(dir, query string) ([]label, error) {
	var stdout bytes.Buffer
	cmd := exec.Command("bazel", "query", "--noshow_progress", query)
	cmd.Stdout = &stdout
	cmd.Dir = dir
	if err := cmd.Run(); err != nil {
		return nil, err
	}
	data := stdout.Bytes()
	var ls []label
	for len(data) != 0 {
		var line []byte
		line, data = splitLine(data)
		i := bytes.IndexByte(line, ':')
		if i < 0 {
			return nil, fmt.Errorf("could not parse label: %q", line)
		}
		ls = append(ls, label{
			pkg:    string(line[:i]),
			target: string(line[i+1:]),
		})
	}
	return ls, nil
}

type rule struct {
	name   string
	models []string
}

func mainE() error {
	nameFlag := flag.String("name", "", "name of rule to add")
	allFlag := flag.Bool("all", false, "add all models")
	scaleFlag := flag.String("scale", "", "scale to apply")
	primitiveColorFlag := flag.Bool("primitive-color", false, "use primitive color")
	normalsFlag := flag.Bool("normals", false, "use normals")
	flag.Parse()
	args := flag.Args()
	wsdir := os.Getenv("BUILD_WORKSPACE_DIRECTORY")
	if wsdir == "" {
		return errors.New("BUILD_WORKSPACE_DIRECTORY not set")
	}
	var models []string
	modelsDir := filepath.Join(wsdir, modelsPath)
	if *allFlag {
		if len(args) > 0 {
			return errors.New("cannot use -all and specify model")
		}
		sts, err := ioutil.ReadDir(modelsDir)
		if err != nil {
			return err
		}
		for _, st := range sts {
			if st.Mode().IsRegular() {
				name := st.Name()
				if !strings.HasPrefix(name, ".") && strings.HasSuffix(name, ".fbx") {
					models = append(models, name)
				}
			}
		}
	} else if len(args) > 0 {
		modelset := make(map[string]bool)
		for _, arg := range args {
			filename := getpath.GetPath(arg)
			relname := getpath.Relative(modelsDir, filename)
			if relname == "" {
				return fmt.Errorf("file %q is not in models directory", arg)
			}
			switch ext := filepath.Ext(relname); ext {
			case ".blend":
				relname = relname[:len(relname)-len(ext)] + ".fbx"
			case ".fbx":
			default:
				return fmt.Errorf("unknown extension: %q", ext)
			}
			if !modelset[relname] {
				modelset[relname] = true
				models = append(models, relname)
			}
		}
	} else {
		return errors.New("no models specified")
	}

	// List existing models.
	ls, err := bazelQuery(modelsDir, "labels(srcs,kind(model,:*))")
	if err != nil {
		return fmt.Errorf("could not list existing models: %v", err)
	}
	oldModels := make(map[string]bool)
	for _, name := range filterPackage(modelsPackage, ls) {
		oldModels[name] = true
	}

	// Filter out existing models.
	var pos int
	for _, model := range models {
		if !oldModels[model] {
			models[pos] = model
			pos++
		}
	}
	models = models[:pos]
	if len(models) == 0 {
		return errors.New("no new models")
	}

	// Generate new rules.
	var rules []rule
	if name := *nameFlag; name != "" {
		rules = []rule{{name: name, models: models}}
	} else {
		for _, model := range models {
			rules = append(rules, rule{
				name:   strings.TrimSuffix(model, ".fbx"),
				models: []string{model},
			})
		}
	}

	// List existing rules.
	ls, err = bazelQuery(modelsDir, "kind(models,:*)")
	if err != nil {
		return fmt.Errorf("could not list existing models: %v", err)
	}
	oldRules := make(map[string]bool)
	for _, name := range filterPackage(modelsPackage, ls) {
		oldRules[name] = true
	}

	// Add to manifest.
	fp, err := os.OpenFile(filepath.Join(wsdir, "assets/manifest.txt"), os.O_WRONLY|os.O_APPEND, 0666)
	if err != nil {
		return fmt.Errorf("could not open manifest: %v", err)
	}
	defer fp.Close()
	for _, model := range models {
		name := strings.TrimSuffix(model, ".fbx")
		if _, err := fmt.Fprintf(fp, "model %s%s models/%s.model",
			identPrefix, strings.ToUpper(name), name); err != nil {
			return fmt.Errorf("could not write to manifest: %v", err)
		}
	}
	if err := fp.Close(); err != nil {
		return fmt.Errorf("could not write to manifest: %v", err)
	}

	// Update build rules.
	scale := *scaleFlag
	primitiveColor := *primitiveColorFlag
	normals := *normalsFlag
	var cmds bytes.Buffer
	var newrules []string
	for _, rule := range rules {
		if !oldRules[rule.name] {
			newrules = append(newrules, rule.name)
			fmt.Fprintf(&cmds, "new models %s|%s/BUILD\n", rule.name, modelsPackage)
			if scale != "" {
				fmt.Fprintf(&cmds, "set scale %q|", scale)
			}
			if primitiveColor {
				fmt.Fprint(&cmds, "set use_primitive_color True")
			}
			if normals {
				fmt.Fprint(&cmds, "set use_normals True")
			}
		}
		cmds.WriteString("add srcs")
		for _, model := range rule.models {
			cmds.WriteByte(' ')
			cmds.WriteString(model)
		}
		fmt.Fprintf(&cmds, "|%s:%s\n", modelsPackage, rule.name)
	}
	if len(newrules) > 0 {
		cmds.WriteString("add srcs")
		for _, name := range newrules {
			fmt.Fprintf(&cmds, " %s:%s", modelsPackage, name)
		}
		cmds.WriteString("|//assets:assets_files\n")
	}
	os.Stdout.Write(cmds.Bytes())
	cmd := exec.Command("buildozer", "-f", "-")
	cmd.Dir = wsdir
	cmd.Stdin = &cmds
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("could not create model build rules: %v", err)
	}

	return nil
}

func main() {
	if err := mainE(); err != nil {
		fmt.Fprintln(os.Stderr, "Error:", err)
		os.Exit(1)
	}
}
