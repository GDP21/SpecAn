#! /usr/bin/env python

import xml.dom.minidom
import sys
import os
from rbuild_support import getXMLstringByTagName, getXMLstringsByTagName, getAttributeOfSingleXMLelement, printWhenVerbose

#######################################################################
def getPreRequisites(userName, publishDir, pkgBuildRoot, cvsCmd='export'):
    """Obtain pre-requisite packages as listed in package.xml
    
    Usage: python getprereq.py userName publishDir pkgBuildRoot [checkout|export]
    
    userName    - CVS user name, only needed for CVS pre-requisite checkouts
    publishDir  - location of the <packageNam>_config.xml file for the primary package
    pkgBuildRoot    - location of package.xml for the primary package
    checkout    - (optional): CVS pre-requisites will be checked out with the CVS checkout command
    export      - (optional, default): CVS pre-requisites will be checked out with the CVS export command
    
    """
    
####################################################
# Documentation of local variables for this function
####################################################
    # userName      CVS user name
    # publishDir    The place where we expect to find pre-built pre-requisites
    # pkgXMLfname   XML that we are parsing to find pre-requisites
    # pkgBuildRoot  Path (within CVS checkout) to find package.xml, and also the  
    #               location where we expect the build output to appear
    #               (pre-requisites are installed on paths relative to this location)
    # toolKit       toolkit version, as a string e.g. 2.3.2b10
    # preReqName    Base name of current pre-requisite (from file)
    # preReqConfig  pre-requisite config (internal, external etc) (from file)
    # preReqBuildNum pre-requisite build number as string (from file)
    # preReqPath    path from pkgBuildRoot to destination of pre-requisite
    # useToolkit    use this attribute to specify version of a toolkit for a pre-requisite
    # reqType       used to setup package name, extenion and extraction type if pre-requisite
    #               wasn't built with rbuild (at the moment only CometApis support added)
    
    pkgXMLfname = pkgBuildRoot + '/package.xml'
    toolKit = os.path.basename(os.getenv('METAG_INST_ROOT'))
    
    print 'Parsing', pkgXMLfname
    doc = xml.dom.minidom.parse(pkgXMLfname)
    printWhenVerbose('Top-level tag is called ' + doc.documentElement.tagName)
    # Pre-built prerequisites
    preReqElements = doc.getElementsByTagName('preBuiltPreRequisite')
    for i, element in enumerate(preReqElements):
        # Get details of a pre-requisite package from the config file
        preReqName = element.getAttribute('name')
        toolKitOption = element.getAttribute('toolKit')
        printWhenVerbose('Pre-requisite ' + str(i) + ' is ' + preReqName + '...')
        preReqConfig = getXMLstringByTagName(element, 'pkgConfig')
        preReqBuildNum = getXMLstringByTagName(element, 'buildNumber')
        preReqDestinationPath = getXMLstringByTagName(element, 'path')
        sourcePath = getXMLstringByTagName(element, 'sourcePath')
        useToolkit = element.getAttribute('useToolkit')
        preReqType = element.getAttribute("reqType")
        
        # Construct package name
        if (toolKitOption == '' or toolKitOption == 'yes'):
            toolKitStr = '_' + toolKit + '_'
        else:
            toolKitStr = '_'
        
        # if ustToolkit attribute is present then set the toolkit name
        if useToolkit:
            toolKitStr = '_' + useToolkit + '_'
            
        pkgName = preReqName + '_' + preReqConfig + toolKitStr + preReqBuildNum + '.tar.gz'
        
        if preReqType == "CometApis":
            pkgName = preReqName + "_" + preReqConfig + "_" + preReqBuildNum + ".zip"
        
        # Get source path
        if (sourcePath == ''):
            # Source path not specified, default to publishDir
            absSourcePath = publishDir
        else:
            # Check if we have been given an absolute source path,
            # or one relative to the publish directory
            pathType = getAttributeOfSingleXMLelement(element, 'sourcePath', 'pathType')
            if (pathType == '' or pathType == 'relative'):
                # default to relative path type
                absSourcePath = publishDir + '/' + sourcePath
            elif (pathType == 'absolute'):
                # absolute path given
                absSourcePath = sourcePath
            else:
                print 'ERROR: Invalid source path type'
                sys.exit(1)
        
        pkgNameWithPath = absSourcePath + '/' + pkgName
        if os.access(pkgNameWithPath, os.F_OK):
            # Package found at the source location
            destinationDir = pkgBuildRoot + '/' + preReqDestinationPath
            if (os.path.exists(destinationDir)):
                printWhenVerbose('Destination directory ' + destinationDir)
            else:
                print 'Destination directory', destinationDir, 'doesn\'t exist, creating..'
                cmd = 'mkdir ' + destinationDir
                retCode = os.system(cmd)
                if (retCode != 0):
                    print 'ERROR: failed to create directory using: ', cmd
                    sys.exit(1)
                
            if os.access(destinationDir + '/' + pkgName, os.F_OK):
                # ... If package already exists at the destination, do nothing
                print pkgName, 'exists at build location already'
            else:
                # ... Otherwise copy and unzip the pre-requisite package
                print 'Fetching pre-built package', pkgName
                os.system('cp ' + pkgNameWithPath + ' ' + destinationDir)
                # ... Only unzip if there isn't already a directory with the pre-requisite
                # name.  This should prevent undesired over-writing when this function is
                # used stand-alone (although note we assume here that the top-level directory
                # within the package is the same as the package base name)
                
                destDirName = destinationDir + '/' + preReqName
                
                if preReqType == "CometApis":
                    destDirName = destinationDir + '/comet_apis'

                if os.path.exists(destinationDir + '/' + preReqName):
                    print 'WARNING: not unzipping pre-requisite', preReqName, \
                        'as directory already present with this name'
                else:
                    if preReqType == "CometApis":
                        print "Unzipping archive ..."
                        os.system('unzip -qq ' + pkgNameWithPath + " -d " + destinationDir)
                    else:
                        print 'Extracting tar ...'
                        os.system('tar -xzf ' + destinationDir + '/' + pkgName + ' -C ' + destinationDir)
        else:
            # Package not found
            print 'ERROR:', pkgName, 'not found'
            sys.exit(1)

    # CVS prerequisites
    if (cvsCmd != 'checkout' and cvsCmd != 'export'):
        print('ERROR: unrecognised CVS command')
        print cvsCmd
        sys.exit(1)
    cvsPreReqElements = doc.getElementsByTagName('cvsPreRequisite')
    for i, element in enumerate(cvsPreReqElements):
        # Get details of a pre-requisite package from the config file
        preReqName = element.getAttribute('name')
        if (preReqName == ''):
            print 'ERROR: name attribute not present for CVS pre-requisite'
            sys.exit(1)
        printWhenVerbose('CVS Pre-requisite ' + str(i) + ' is ' + preReqName + '...')
        preReqDestinationPath = getXMLstringByTagName(element, 'path')
        if (preReqDestinationPath == ''):
            print 'ERROR: destination path not specified for CVS pre-requisite', preReqName
            sys.exit(1)
        destinationDir = pkgBuildRoot + '/' + preReqDestinationPath
        printWhenVerbose('Destination directory ' + destinationDir)
        versionTag = getXMLstringByTagName(element, 'version')
        if (versionTag == ''):
            print 'WARNING: version for CVS pre-requisite ', preReqName, 'defaults to HEAD'
            versionTag = 'HEAD'
        else:
            printWhenVerbose('Checking out on tag ' + versionTag)
        cvsRoot = getXMLstringByTagName(element, 'cvsRoot')
        if (cvsRoot == ''):
            print 'ERROR: CVS root not specified for CVS pre-requisite', preReqName
            sys.exit(1)
        pserverString = ':pserver:' + userName + '@' + cvsRoot
        moduleName = getXMLstringByTagName(element, 'module')
        if (moduleName == ''):
            print 'ERROR: module name not specified for CVS pre-requisite', preReqName
            sys.exit(1)
        
        # If we are at the head then checkout with no tag rather than using HEAD as a tag.
        # This avoids HEAD being treated as a sticky tag.
        if (versionTag == 'HEAD' and cvsCmd == 'checkout'):
            branchStr = ''
        else:
            branchStr = ' -r ' + versionTag

        # Check out package at destination location
        
        if (os.path.exists(destinationDir)):
            printWhenVerbose('Destination directory ' + destinationDir)
        else:
            print 'Destination directory', destinationDir, 'doesn\'t exist, creating..'
            cmd = 'mkdir ' + destinationDir
            retCode = os.system(cmd)
            if (retCode != 0):
                print 'ERROR: failed to create directory using: ', cmd
                sys.exit(1)
        
        print 'Checking out ', preReqName
        #cmd = 'cd ' + destinationDir
        #retCode = os.system(cmd)
        
        currentDir = os.getcwd()
        os.chdir(destinationDir)
            
        cmd = 'cvs -z9 -Q -d ' + pserverString + ' ' + cvsCmd + branchStr + ' -d ' + preReqName + ' ' + moduleName
        retCode = os.system(cmd)
        if (retCode != 0):
            print 'ERROR: FAILED CHECKOUT using: ', cmd
            sys.exit(1)
            
        # Change back to original directory
        os.chdir(currentDir)


#######################################################################

#
# This code enables this module to be run stand-alone
#
if __name__ == "__main__":
    numParams = len(sys.argv) - 1
    if (numParams == 3):
        getPreRequisites(sys.argv[1], sys.argv[2], sys.argv[3], 'export')
    elif (numParams == 4):
        getPreRequisites(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print getPreRequisites.__doc__

