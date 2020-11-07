#!/bin/sh
set -e
bazel build --platforms=//n64 :rom_images
bazel test :tests
