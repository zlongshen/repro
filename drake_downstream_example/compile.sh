#!/bin/bash
set -e -u

ext=$(dirname $PWD)/drake_package_ext
( cd $ext && ./compile.sh )

export PATH=$ext/build/install/bin:$PATH
mkdir -p build
cd build
cmake ..
make