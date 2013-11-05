#! /bin/bash

# 
# Inclusions
#

source rbuild_debug.bash
source rbuild_xml.bash

#######
### Test input parameters.

MINPARAMS=3
MAXPARAMS=3

if [ "$#" -lt "$MINPARAMS" -o "$#" -gt "$MAXPARAMS" -o "$1" = "--help" ]
then
	echo "This script obtains the pre-requisites required by the package.";
	echo "";
	echo "It will read package.xml and reap pre-requisites either from the package";
	echo "publish directory, or from source control such as CVS."
	echo "";
	echo "";
	echo "Usage: ${0##*/} [userName] [publishDir] [packageXML]";
	echo "";
	echo "userName    - CVS username. YOU MUST LOG IN PRIOR TO RUNNING THIS SCRIPT.";
	echo "publishDir  - Path to the publish directory.";
	echo "packageXML  - Path and name of package.xml file.";
	exit;
fi


#######
### Parse input parameters.

CVS_USERNAME=$1
PUBLISH_ROOT=$2
PACKAGE_XML=$3


PACKAGE_XML_DIR=$(dirname $PACKAGE_XML)
PACKAGE_XML_NAME=$(basename $PACKAGE_XML)

debug 2 $CVS_USERNAME 
debug 2 $PUBLISH_ROOT
debug 2 $PACKAGE_XML_DIR
debug 2 $PACKAGE_XML_NAME

#
# If this script is being called from rbuild.bash then inherit PKG_BUILD_ROOT
# otherwise set it to the PACKAGE_XML_DIR
#

if [ "$PKG_BUILD_ROOT" == "" ]; then
PKG_BUILD_ROOT=$PACKAGE_XML_DIR
fi

echo $PKG_BUILD_ROOT;

if [ -e $PACKAGE_XML_DIR/$PACKAGE_XML_NAME ]; then  
	displayInfo "Parsing package.xml ...";

else
	displayInfo "$PACKAGE_XML_NAME in $PACKAGE_XML_DIR/ doesn't exist ...";
	exit 1;

fi


### End input parameter parsing.
#######



### End parameters testing.
#######


#
# Parse package.xml file
#

extractElementContents preBuiltPreRequisite $PACKAGE_XML_DIR/$PACKAGE_XML_NAME
PRE_BUILT_PREREQ_LIST=( "${RESULT[@]}" )

extractElementContents sourcePreRequisite $PACKAGE_XML_DIR/$PACKAGE_XML_NAME
SOURCE_PREREQ_LIST=( "${RESULT[@]}" )


# Iterate through each pre-requisite

TOOLKIT=$(basename $METAG_INST_ROOT)

cnt=${#PRE_BUILT_PREREQ_LIST[@]}

for (( i = 0 ; i < cnt ; i++ ))
do
    extractStringAttributeFromString name "${PRE_BUILT_PREREQ_LIST[$i]}"
    PRE_BUILT_PREQ_NAME=$RESULT
    
    extractElementFromString pkgConfig "${PRE_BUILT_PREREQ_LIST[$i]}"
    PRE_BUILT_PREQ_CONFIG=$RESULT
    extractElementFromString buildNumber "${PRE_BUILT_PREREQ_LIST[$i]}"
    PRE_BUILT_PREQ_BUILD_NUM=$RESULT
    extractElementFromString path "${PRE_BUILT_PREREQ_LIST[$i]}"
    PRE_BUILT_PREQ_DEST_PATH=$RESULT

    # Construct package name
    PRE_BUILT_PREQ_TAR_NAME="$PRE_BUILT_PREQ_NAME"_"$PRE_BUILT_PREQ_CONFIG"_"$TOOLKIT"_"$PRE_BUILT_PREQ_BUILD_NUM".tar.gz


    if [ -e $PUBLISH_ROOT/$PRE_BUILT_PREQ_TAR_NAME ]; then  
    
        if [ ! -e $PKG_BUILD_ROOT/$PRE_BUILT_PREQ_DEST_PATH/$PRE_BUILT_PREQ_TAR_NAME ]; then
            displayInfo "Copying $PRE_BUILT_PREQ_TAR_NAME ...";
            cp $PUBLISH_ROOT/$PRE_BUILT_PREQ_TAR_NAME $PKG_BUILD_ROOT/$PRE_BUILT_PREQ_DEST_PATH
            displayInfo "Extracting tar ...";
            pushd .
            cd $PKG_BUILD_ROOT/$PRE_BUILT_PREQ_DEST_PATH
        
            echo `pwd`
            tar -xzf $PRE_BUILT_PREQ_TAR_NAME
            popd
        
        else
            displayInfo "$PRE_BUILT_PREQ_TAR_NAME exists already ...";
        fi
        
    else
        displayInfo "$PRE_BUILT_PREQ_TAR_NAME in $PUBLISH_ROOT/ doesn't exist ...";
        exit 1;

    fi
    

done



exit 0;