#!/bin/sh
set -e
bazel build --platforms=//n64 :rom_images :tests
bazel test :tests
