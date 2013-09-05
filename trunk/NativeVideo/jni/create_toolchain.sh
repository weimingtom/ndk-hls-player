#!/bin/bash
pushd `dirname $0`
. settings.sh

echo $NDK

$NDK/build/tools/make-standalone-toolchain.sh --install-dir=./toolchain
