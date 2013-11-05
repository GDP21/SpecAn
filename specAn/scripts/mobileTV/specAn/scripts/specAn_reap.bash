#!/bin/bash
# Script to reap the complete file/directory structure for building the Spectrum Analyser project

MINPARAMS=2
MAXPARAMS=4

if [ "$#" -lt "$MINPARAMS" -o "$#" -gt "$MAXPARAMS" -o "$1" = "--help" ]
then
	echo "This script sets up the complete file/directory structure for building the Spectrum Analyser project"
	echo "Usage: ${0##*/} userName publishDir [tag] [rbuildPath]";
	echo "	userName:	Your user name for Chepstow CVS repository"
	echo "	publishDir:	Path to the Satellite rbuild config file e.g /cygdrive/k/Packages/Satellite if on cygwin"
	echo "  tag: 		(Optional) CVS tag for primary package (defaults to the head)"
	echo "	rbuildPath:	(Optional) Path to find getprereq.py.  If absent the script will check out rbuild prior to running."
	echo "The directory structure will be created at the point where the script is run."
	exit;
fi

userName=$1
publishDir=$2

ENS_CVS_PATH=:pserver:$userName@chCVS.ch.imgtec.org:/cvs

# Obtain tag from command line if present
if [ $# -gt "$MINPARAMS" ]
then
	CVSTAG=-r$3
fi

# User can set a path to existing rbuild on command line, otherwise check it out at top level
if [ $# -eq "$MAXPARAMS" ]
then
    rbuildPath=$4
else
    cvs -d $ENS_CVS_PATH checkout -d rbuild ensigmaSDK/rbuild
    rbuildPath=rbuild
fi

# Check out primary package at the head
cvs -d $ENS_CVS_PATH checkout $CVSTAG mobileTV/specAn
cvs -d $ENS_CVS_PATH checkout $CVSTAG -N mobileTV/support/specAn # -N option preserves full path
cvs -d $ENS_CVS_PATH checkout -N -l mobileTV/common # This gives us just the common directory with nothing below it

# Use getprereq to get pre-requisite packages; these will be brought out on tags according to the settings in package.xml
python ${rbuildPath}/getprereq.py $userName $publishDir mobileTV/support/specAn/package/build/smake checkout


