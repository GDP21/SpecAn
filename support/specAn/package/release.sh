#!/bin/bash -e

#
# Release script for Spectrum Analyser package
#

#############################################################################
#
# Functions
#

function logStage
{
    echo "====================================================================="
    echo "$1 ..."
    echo "====================================================================="
}

function extractPkgs
{
    rm -rf temp
    mkdir temp
    logStage "Extracting packages"
    pkgs=`ls *.tar.gz | grep $PKG_CONFIG`
    cd temp
    for pkg in $pkgs; do
        tar -xzf ../$pkg
        dir=`echo $pkg | sed 's,.tar.gz,,'`
        mv $DISTRIBUTION_NAME $dir
        rm ../$pkg
    done
    cd ..
}

function preBuild
{
    logStage "Pre-building"
    cd temp
    find -mindepth 1 -maxdepth 1 -type d | while read dir; do
        cd $dir
        topDir=`pwd`
        cd support/specAn/loaders/demoApp/build/smake
        objSub=release_METAGCC
        binDir=$topDir/bin/$objSub
        if [ ! -e $binDir ]; then mkdir -p $binDir; fi
        make CONFIG=release TARGET=$PKG_TARGET UCCP=$PKG_UCCP FEATURES=$PKG_FEATURES all
        for match in t0.elf .js .py .ldr; do
            cp $objSub/*$match $binDir
        done
        # Clobber the app. This is so we leave the source tree in
        # a fresh state.
        make CONFIG=release clobber
        cd $topDir/..
    done
    cd ..
}


function compressPkgs
{
    logStage "Compressing packages"
    cd temp
    find -mindepth 1 -maxdepth 1 -type d | while read dir; do
        mv $dir $DISTRIBUTION_NAME
        pkg=${DISTRIBUTION_NAME}_${PKG_CONFIG}.tar.gz
        tar -czf $pkg $DISTRIBUTION_NAME
        mv $pkg ..
        rm -rf $DISTRIBUTION_NAME
    done
    cd ..
}


#############################################################################
#
# Main script
#

# param1 - Distribution package's primary name
# param2 - Package configuration

DISTRIBUTION_NAME=$1
PKG_CONFIG=$2

#
# Examine the specified package config.
# Do nothing on an external config. Trap invalid configs.
# The "internal" build is a package to run on the Saturn bring-up board,
# containing both host port app and mainApp, for testing purposes.
#
case "$PKG_CONFIG" in
    "internal") PKG_TARGET=SATURN2_BUB PKG_UCCP=330_3 PKG_FEATURES= ;;
    "saturn2_generic_tuner") PKG_TARGET=SATURN2_BUB_NOTUNER PKG_UCCP=330_3 PKG_FEATURES=STATIC_MSGBUF;;
    "meta_src_mcp_bin_shannon_config005") PKG_TARGET=EXAMPLE PKG_UCCP=330_5 PKG_FEATURES= ;;
    "meta_src_mcp_bin_shannon_config012") PKG_TARGET=EXAMPLE PKG_UCCP=330_12 PKG_FEATURES= ;;
    "meta_src_mcp_bin_shannon_config016") PKG_TARGET=EXAMPLE PKG_UCCP=330_16 PKG_FEATURES=STATIC_MSGBUF;;
    "external") exit 0;;
    *) echo "Invalid package configuration ($PKG_CONFIG)"; exit 1;;
esac

#
# Ripple through the distribution name
#
export UNIT_NAME=$DISTRIBUTION_NAME

#
# Clobber
#
logStage "Clobbering"
cd build/smake
make CONFIG=release TARGET=$PKG_TARGET PKG_CONFIGURATION=$PKG_CONFIG clobber
if [ $? != 0 ]; then
    exit 1;
fi

#
# Build
#
logStage "Building"
make CONFIG=release TARGET=$PKG_TARGET PKG_CONFIGURATION=$PKG_CONFIG all

# Extract packages
extractPkgs

# Re-build to produce a pre-built binary for the bin directory
preBuild

# Compress packages
compressPkgs
