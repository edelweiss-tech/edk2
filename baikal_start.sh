#!/bin/bash

export EDK_TOOLS_PATH=`pwd`/BaseTools
echo "EDK_TOOLS_PATH:" $EDK_TOOLS_PATH 
#export CROSS_COMPILE=$HOME/cdk/gcc-linaro-aarch64-linux-gnu-4.7-2013.04-20130415_linux/bin/aarch64-linux-gnu-
export CROSS_COMPILE=`pwd`/../../xtools/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-
export GCC6_AARCH64_PREFIX=$CROSS_COMPILE
cd $EDK_TOOLS_PATH/..
. ./edksetup.sh BaseTools
