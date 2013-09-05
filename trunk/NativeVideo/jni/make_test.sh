#!/bin/bash
pushd `dirname $0`
. settings.sh
pushd test
make 
popd; popd
