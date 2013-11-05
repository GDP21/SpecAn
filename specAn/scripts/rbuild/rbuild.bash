#! /bin/bash

# 
# Inclusions
#

source rbuild_debug.bash
source rbuild_xml.bash

#######
### Test input parameters.

MINPARAMS=4
MAXPARAMS=7
BADPARAMS=5

if [ "$#" -lt "$MINPARAMS" -o "$#" -gt "$MAXPARAMS" -o "$#" == "BADPARAMS" -o "$1" = "--help" ]
then
	echo "This script does a Release build for a package.";
	echo "";
	echo "It will read publishDir/packageName_config.xml & perform the following actions:";
	echo "";
	echo "- Reap the package to be released (primary package) from CVS.";
	echo "- Determine any dependencies and retrieve any supporting packages.";
	echo "- Execute a release script for the default internal & external configurations.";
	echo "- Execute a release script for any configurations specifed in the config file.";
	echo "- Rename the resulting tars with the current build number.";
	echo "- Publish the renamed tars to the publish directory.";
	echo "- Publish the resulting build logs to the publish directory.";
	echo "- Tag the primary package source with the current build number.";
	echo "";
	echo "Usage: ${0##*/} [mode] [userName] [publishDir] [packageName] [configDir] [branch]";
	echo "";
	echo "mode        - publish or test or info.";
	echo "userName    - CVS username. YOU MUST LOG IN PRIOR TO RUNNING THIS SCRIPT.";
	echo "publishDir  - Path to the publish directory.";
	echo "packageName - Name of the package to be released.";
	echo "cvsRoot     - Configuration file CVS root. Optional, however requires [configMod] and [branch] to be defined";
	echo "configMod   - CVS module name giving the location of the configuration file."; 
	echo "branch      - Branch to release or HEAD.";
	exit 1;
fi


#######
### Parse input parameters.

RBUILD_MODE=$1
CVS_USERNAME=$2
PUBLISH_ROOT=$3
PRIMARY_PACKAGE_NAME=$4

if [ "$#" == "$MAXPARAMS" ]; then
	CONFIG_CVS_ROOT=$5
	CONFIG_MODULE=$6
	CONFIG_ROOT=`pwd`/$CONFIG_MODULE
	BRANCH=$7
	if [ "$BRANCH" != "HEAD" ]; then
		CVS_REVISION_CMD="-r $BRANCH"
	else
		CVS_REVISION_CMD=""
	fi
	
	cvs -z9 -Q -d :pserver:$CVS_USERNAME@$CONFIG_CVS_ROOT checkout $CVS_REVISION_CMD -P $CONFIG_MODULE
else
	CONFIG_CVS_ROOT=""
	CONFIG_ROOT=$PUBLISH_ROOT
	CONFIG_MODULE=""
	BRANCH="HEAD"
	CVS_REVISION_CMD=""
fi

RBUILD_CONFIG_XML_FILE="$PRIMARY_PACKAGE_NAME"_config.xml
ABS_RBUILD_CONFIG_XML_FILE=$CONFIG_ROOT/$RBUILD_CONFIG_XML_FILE

TOOLKIT=$(basename $METAG_INST_ROOT)

debug 2 $RBUILD_MODE
debug 2 $PUBLISH_ROOT 
debug 2 $PRIMARY_PACKAGE_NAME
debug 2 $ABS_RBUILD_CONFIG_XML_FILE

if [ "$CONFIG_CVS_ROOT" != "" ]; then
	debug 2 $CONFIG_CVS_ROOT
	debug 2 $CONFIG_MODULE
	debug 2 $BRANCH
fi


#
# Test for valid mode
#
if [ "$RBUILD_MODE" == 'test' ]; then
        displayInfo "Test mode only - No publishing or cvs tagging."	
else
	if [ "$RBUILD_MODE" == 'publish' ]; then
		displayInfo "Publish mode."
	else
		if [ "$RBUILD_MODE" == 'info' ]; then
			displayInfo "Info only mode."
		else
			displayInfo "Illegal mode: $RBUILD_MODE";
			displayInfo "Try running rbuild.bash with no arguments for help."
			exit 1;
		fi
	fi
fi


#
# Check for the existance of _config.xml and move to local directory
#
if [ -e $ABS_RBUILD_CONFIG_XML_FILE ]; then cp -f $ABS_RBUILD_CONFIG_XML_FILE .; 
else
	displayInfo "$ABS_RBUILD_CONFIG_XML_FILE doesn't exist. Can't continue.";
	exit 1;
fi


### End input parameter parsing.
#######



### End parameters testing.
#######


#
# Parse rbuild_config.xml file
#

#
# Obtain distribution name
#
extractStringAttribute publishName $RBUILD_CONFIG_XML_FILE
PRIMARY_PACKAGE_NAME=$RESULT
debug 3 $PRIMARY_PACKAGE_NAME

# Obtain CVS build number
extractBuildNumberFinalIntegerAttribute buildNumber $RBUILD_CONFIG_XML_FILE
if [ "$RESULT" != "" ]; then
	BUILD_NUMBER=$RESULT
    NEXT_BUILD_NUMBER=$(($BUILD_NUMBER + 1))
	extractBuildNumberShortAttribute buildNumber $RBUILD_CONFIG_XML_FILE
	BUILD_NUMBER_SHORT=$(echo $RESULT | tr -d [:space:])
else
	extractBuildNumberAttribute buildNumber $RBUILD_CONFIG_XML_FILE
	BUILD_NUMBER=$RESULT
    NEXT_BUILD_NUMBER=$(($RESULT + 1))
	BUILD_NUMBER_SHORT=""
fi

LAST_BUILD_NUMBER=$BUILD_NUMBER_SHORT$BUILD_NUMBER
THIS_BUILD_NUMBER=$BUILD_NUMBER_SHORT$NEXT_BUILD_NUMBER

displayInfo "Last build: $LAST_BUILD_NUMBER"
displayInfo "This build: $THIS_BUILD_NUMBER"


# Obtain CVS root
extractElement cvsRoot $RBUILD_CONFIG_XML_FILE
CVS_ROOT=$RESULT
debug 1 $CVS_ROOT 

# Obtain CVS path
extractElement cvsLocation $RBUILD_CONFIG_XML_FILE
CVS_PATH_LIST=$RESULT
debug 1 $CVS_PATH_LIST

# Obtain package build root
extractElement pkgBuildRoot $RBUILD_CONFIG_XML_FILE
export PKG_BUILD_ROOT=$RESULT
debug 1 $PKG_BUILD_ROOT

# Obtain package script
extractElement pkgReleaseScript $RBUILD_CONFIG_XML_FILE
PKG_SCRIPT=$RESULT
debug 1 $PKG_SCRIPT

# Obtain package configurations
extractElement pkgConfig $RBUILD_CONFIG_XML_FILE
PKG_CONFIG_LIST=$RESULT
debug 1 "$PKG_CONFIG_LIST"

# Gernerate CVS root path
CVS_ROOT_PATH=:pserver:"$CVS_USERNAME"@"$CVS_ROOT"


#
# Setup top directory path
#
TOP_DIR=`pwd`


#
# Test the build number and CVS to determine if required to build new package.
#
if [ "$LAST_BUILD_NUMBER" == "0" ]; then

    displayInfo "Building for the first time ... ";
    
else

    #
    # Determine difference since last build
    #
    displayInfo "Generate change log ... "

	CVS_LOG_STRING=RBUILD_"$PRIMARY_PACKAGE_NAME"_"$LAST_BUILD_NUMBER"::"$BRANCH"
	
	cvs -d$CVS_ROOT_PATH -Q rlog -NS -r$CVS_LOG_STRING $CVS_PATH_LIST > change_log.txt
	
    CHANGE_LOG_CONTENTS=`more change_log.txt`

    if [ "$CHANGE_LOG_CONTENTS" == '' ]; then 
        displayInfo "No source changes since last build.";
        if [ -e $PUBLISH_ROOT/"$PRIMARY_PACKAGE_NAME"_internal_"$TOOLKIT"_build_"$LAST_BUILD_NUMBER".tar.gz ]; then
            displayInfo "The toolkit has not changed.";
            displayInfo "No need to continue.";
            exit 0;
        else 
            displayInfo "Building with new toolkit $TOOLKIT";
        fi; 
    fi;
fi;


if [ "$RBUILD_MODE" == 'info' ]; then
    displayInfo "Info only mode finished ...";
    exit 0;
fi;


#
# Buiuld CVS TAG
#
if [ "$BRANCH" == "HEAD" ]; then
	CVS_TAG=`echo RBUILD_"$PRIMARY_PACKAGE_NAME"_"$THIS_BUILD_NUMBER" | sed 's/\./_/g'`
else
	CVS_TAG=`echo RBUILD_"$PRIMARY_PACKAGE_NAME"_"$THIS_BUILD_NUMBER"_"$BRANCH" | sed 's/\./_/g'`
fi

#
# Build CVS command to delete TAG if an error occurs.
#
CVS_UNTAG_CMD="cvs -Q -d $CVS_ROOT_PATH rtag -d $CVS_TAG $CVS_PATH_LIST"


#
# TAG
#
displayInfo "Tagging $CVS_PATH_LIST with $CVS_TAG ... "
cvs -Q -d $CVS_ROOT_PATH rtag $CVS_REVISION_CMD $CVS_TAG $CVS_PATH_LIST


# 
# REAP
#
displayInfo "Checking out $CVS_PATH_LIST ..."
displayInfo ""

CVS_CMD="cvs -z9 -Q -d $CVS_ROOT_PATH checkout -r $CVS_TAG -P $CVS_PATH_LIST"

eval $CVS_CMD

if [ $? != 0 ]; then
	displayInfo ""
	displayInfo "FAILED CHECKOUT! using:"
        displayInfo "$CVS_CMD"
	# Remove TAG if error 
	eval $CVS_UNTAG_CMD
	exit 1;
fi


#
# Process package.xml 
#
displayInfo "Looking for package.xml";

foundPackageXML=0;

if [ -e $PKG_BUILD_ROOT/package.xml ]; then  
	displayInfo "Found package.xml in $PKG_BUILD_ROOT/";
	foundPackageXML=1;
fi

if [ "$foundPackageXML" == '1' ]; then
    
    displayInfo "Obtain pre-requisites ...";
	
    ./rbuild_obtain_prerequisites.bash $CVS_USERNAME $PUBLISH_ROOT $PKG_BUILD_ROOT/package.xml

    if [ $? != 0 ]; then
        displayInfo ""
        displayInfo "FAILED TO OBTAIN PRE-REQUISITES!"
       	# Remove TAG if error 
		eval $CVS_UNTAG_CMD
		exit 1;
    fi
else
	displayInfo "$PKG_BUILD_ROOT/package.xml doesn't exist.";
fi


RELEASE_SCRIPT_DIR=$(dirname $PKG_SCRIPT)
RELEASE_SCRIPT=$(basename $PKG_SCRIPT)

debug 2 $RELEASE_SCRIPT_DIR
debug 2 $RELEASE_SCRIPT


#
# Execute package release script 
#

if [ -e $RELEASE_SCRIPT_DIR/$RELEASE_SCRIPT ]; then  
# Move to $RELEASE_SCRIPT_DIR
        displayInfo "Moving to $RELEASE_SCRIPT_DIR ..."
	cd $RELEASE_SCRIPT_DIR
	displayInfo "Processing package release script"
	chmod +x $RELEASE_SCRIPT
# Execute package script

# Default configurations - internal and external
    PKG_CONFIG_LIST="internal external $PKG_CONFIG_LIST"
	
# Iterate through configurations 
	for pkgConfig in $PKG_CONFIG_LIST; do 
		displayInfo "Executing package script: $pkgConfig"
		./$RELEASE_SCRIPT $PRIMARY_PACKAGE_NAME $pkgConfig >& $TOP_DIR/package_script_"$pkgConfig"_log.txt

		if [ $? != 0 ]; then
			displayInfo ""
			displayInfo "ERROR occured within package script."
			# Remove TAG if error 
			eval $CVS_UNTAG_CMD
			exit 1;
		fi
	done
		
else
	displayInfo "Package release script $RELEASE_SCRIPT_DIR/$RELEASE_SCRIPT doesn't exist."
	# Remove TAG if error 
	eval $CVS_UNTAG_CMD
	cd $TOP_DIR
	exit 1
fi

cd $TOP_DIR

#
# Move tars to top dir
#
displayInfo "Copying tars...."
cp -f $PKG_BUILD_ROOT/*.tar.gz $TOP_DIR



#
# tar up logs
#
displayInfo "tar up logs ..."
BUILD_LOG_TAR="$PRIMARY_PACKAGE_NAME"_log.tar.gz
rm -f $BUILD_LOG_TAR
tar -zcvf $BUILD_LOG_TAR *log.txt
rm -f *log.txt

#
# Add build number to tars
#
displayInfo "Adding build number to tars ...."
for name in `ls $PRIMARY_PACKAGE_NAME*.tar.gz`;do 
	NEW_NAME=`echo $name | sed 's/.tar.gz/_build_'$THIS_BUILD_NUMBER'.tar.gz/g'`
	mv -f $name $NEW_NAME
done


#
# Only tag and publish if RBUILD_MODE = publish
#
if [ "$RBUILD_MODE" == 'publish' ]; then

#
# Update build number in rbuild_config.xml file
#
	chmod +w $RBUILD_CONFIG_XML_FILE
	SED_COMMAND='s/buildNumber="'$LAST_BUILD_NUMBER'"/buildNumber="'$THIS_BUILD_NUMBER'"/g'
	sed -i $SED_COMMAND $RBUILD_CONFIG_XML_FILE
	chmod +w $ABS_RBUILD_CONFIG_XML_FILE
	mv -f $RBUILD_CONFIG_XML_FILE $CONFIG_ROOT
	chmod -w $ABS_RBUILD_CONFIG_XML_FILE
	if [ "$CONFIG_CVS_ROOT" != "" ]; then
		cvs -Q -d $CVS_ROOT_PATH commit -m"Auto" $ABS_RBUILD_CONFIG_XML_FILE
		cvs -Q -d $CVS_ROOT_PATH tag $CVS_TAG $ABS_RBUILD_CONFIG_XML_FILE
	fi 


#
# Publish
#
	displayInfo "Publishing ..."
	for name in `ls $PRIMARY_PACKAGE_NAME*.tar.gz`;do 
		displayInfo "Publishing $name"
		chmod -w $name
		cp -f $name $PUBLISH_ROOT 
	done

else

#
# Remove TAG if runing a test 
#
	eval $CVS_UNTAG_CMD

fi


exit 0
