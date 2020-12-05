#!/bin/sh
set -e
bazel build -c opt --platforms=//n64 :rom_images :tests
bazel test :tests
