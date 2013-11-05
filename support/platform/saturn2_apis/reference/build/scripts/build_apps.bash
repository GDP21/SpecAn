#!/bin/bash
#
# FILE NAME:   build_apps.bash
#
# PROJECT:	   PowerVR Tools
#
# AUTHOR:      Imagination Technologies
#
# DESCRIPTION: Build all apps.
#
###############################################

#######
### Test input parameters and save user names
MINPARAMS=1
MAXPARAMS=2

if [ "$#" -lt "$MINPARAMS" -o "$#" -gt "$MAXPARAMS" -o "$1" = "--help" ]
then
	echo ""
	echo "This script builds the application below a given directory."
	echo ""
	echo "Usage: ${0##*/} directory [arguments]"
	echo ""
	echo "Options:"
	echo ""
	echo "    directory - The directory containing the applications. The script"
	echo "                searches for /"build/" directories below the specified"
	echo "                directory and invokes /"make/" from within the /"build/""
	echo "                directory."
	echo ""
	echo "    arguments - Optional arguments passed to /"make/""
	echo ""
	echo "Parameters Given:"
	while (( "$#" )); do
		echo $1
		shift
	done
	echo ""
	exit;
fi

TOP_DIR=$1
MAKE_ARGS=$2


###############################################
##
##	Get the list of dirs.
##
DIRS=`find ${TOP_DIR} -mindepth 1 -maxdepth 2 -type d \-name build`
		
###############################################
##
##	Tag the files.
##

for dir in ${DIRS}
do
	(cd ${dir}; pwd; make $MAKE_ARGS)
done

### END OF FILE ###
