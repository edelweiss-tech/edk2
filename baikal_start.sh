#!/bin/bash

export EDK_TOOLS_PATH=`pwd`/BaseTools
echo "EDK_TOOLS_PATH:" $EDK_TOOLS_PATH 
export CROSS_COMPILE=`pwd`/../../xtools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-
export GCC6_AARCH64_PREFIX=$CROSS_COMPILE
cd $EDK_TOOLS_PATH/..
. ./edksetup.sh BaseTools
