#! /usr/bin/env python

import xml.dom.minidom
import sys
import os
import shutil
import fnmatch
import re
from getprereq import getPreRequisites
from rbuild_support import getXMLstringByTagName, getXMLstringsByTagName, getAttributeOfSingleXMLelement, printWhenVerbose, getElementsAttributeDict

#######################################################################
# Main program
#######################################################################

#######
### Test input parameters.

minParams = 4
maxParams = 5

numParams = len(sys.argv) - 1

if (numParams < minParams) or (numParams > maxParams) or (sys.argv[1] == '--help'):
	print """
This script does a Release build for a package.

It will read publishDir/packageName_config.xml & perform the following actions:

- Reap the package to be released (primary package) from CVS.
- Determine any dependencies and retrieve any supporting packages.
- Execute a release script for the default internal & external configurations.
- Execute a release script for any configurations specifed in the config file.
- Rename the resulting tars with the current build number.
- Publish the renamed tars to the publish directory.
- Publish the resulting build logs to the publish directory.
- Tag the primary package source with the current build number.


Usage: python rbuild.py [mode] [userName] [publishDir] [packageName] [branchTag]

mode        - publish or test or info.
userName    - CVS username. YOU MUST LOG IN PRIOR TO RUNNING THIS SCRIPT.
publishDir  - Path to the publish directory.
packageName - Name of the package to be released.
branchTag	- CVS branch tag to use for primary package reap.  This parameter
			is optional in which case CVS HEAD will be used
"""
	sys.exit(1)

#######################################
# Documentation of main local variables
#######################################
# rbuildMode		publish, test or info as passed on command line
# cvsUserName		CVS user name
# primaryPackageName The name of the package as given on command line and in the
#					config file (the latter over-writes the former)
# publishRoot		The directory in which we expect to find <primaryPackageName>_config.xml,
#					and where we expect to publish our package to (and find dependencies)
#					- as passed on command line
# rBuildConfigXMLfile <primaryPackageName>_config.xml
# confFileWithPath	<primaryPackageName>_config.xml including path
# toolKit			Meta toolkit version gleaned from environment variable
# lastBuildNum		Number of last build as a string
# thisBuildNum		Number of this build
# cvsRoot			CVS repository for checkout of this package (from config file)
# cvsPath			Path to this module in CVS
# pkgBuildRoot		Path (within CVS checkout) to find package.xml, and also the location 
#					where we expect the build output to appear
# pkgReleaseScript	Release script name including path (from config file)
# pkgConfigList		List of different package configurations (from config file or default)



#######
### Parse input parameters.

rbuildMode = sys.argv[1]
cvsUserName = sys.argv[2]
publishRoot = sys.argv[3]
primaryPackageName = sys.argv[4]
if (numParams >= 5 and sys.argv[5] != 'HEAD'):
	branchTag = '-r ' + sys.argv[5]
else:
	branchTag = ''

rBuildConfigXMLfile = primaryPackageName + '_config.xml'
confFileWithPath = publishRoot + '/' + rBuildConfigXMLfile
printWhenVerbose('Config XML file: ' + confFileWithPath)

metag_inst_root = os.getenv('METAG_INST_ROOT')
if (metag_inst_root == None):
	toolKit = ''
	printWhenVerbose('Toolkit environment variable not set')
else:
	toolKit = os.path.basename(metag_inst_root)
	printWhenVerbose('Toolkit: ' + toolKit)

#
# Test for valid mode
#
if rbuildMode == 'test':
	print 'Test mode only - No publishing or cvs tagging.'
elif rbuildMode == 'publish':
	print 'Publish mode.'
elif rbuildMode == 'info':
	print 'Info only mode.'
else:
	print 'ERROR: Illegal mode:', rbuildMode
	print 'Try running rbuild.py with no arguments for help.'
	sys.exit(1)
	
#
# Check for the existance of _config.xml and move to local directory
# TODO don't really see the need for this, could just parse it where it is?
#
if os.path.exists(confFileWithPath):
	# Note that the following line encounters permissions problems (need to ensure we
	# will be able to over-write local copy if re-run).  For now, use the OS command instead.
	# shutil.copy(confFileWithPath, '.')
	cmd = 'cp -f ' + confFileWithPath + ' .'
	printWhenVerbose('Executing ' + cmd)
	os.system(cmd)
else:
	print "ERROR:", confFileWithPath, 'doesn\'t exist. Can\'t continue.'
	sys.exit(1)


### End input parameter parsing.
#######


### End parameters testing.
#######


#
# Parse (local copy of) <primaryPackageName>_config.xml file
#
doc = xml.dom.minidom.parse(rBuildConfigXMLfile)
printWhenVerbose('Top-level tag is called ' + doc.documentElement.tagName)
	
# Obtain distribution name (an attribute of the top-level element).
# Note this over-writes the name that
# we were passed on the command line; the two will normally be the same.
primaryPackageName = doc.documentElement.getAttribute('publishName')
printWhenVerbose('Publish name ' + primaryPackageName)

# Get addDefaultConfigs attribute of top-level element, this tells us how to
# interpret <pkgConfig> later.
addDefaultConfigs = doc.documentElement.getAttribute('addDefaultConfigs')

# Obtain CVS root
cvsRoot = getXMLstringByTagName(doc, 'cvsRoot')

# Determine if the CVS repository is accessed via a local path, or using
# :pserver: (the latter is the normal system for a networked repository, and the default)
local_cvs = 0	# default
localCVSattr = getAttributeOfSingleXMLelement(doc, 'cvsRoot', 'local_cvs')
if localCVSattr == '':
	printWhenVerbose('local_cvs attribute not present')
elif localCVSattr == 'yes':
	printWhenVerbose('local CVS')
	local_cvs = 1
elif localCVSattr == 'no':
	printWhenVerbose('non-local CVS')
else:
	print 'ERROR: Unrecognised local_cvs attribute'
	sys.exit(1)

# Obtain CVS path
cvsLocation = getXMLstringsByTagName(doc, 'cvsLocation')

cvsLocationsDict = getElementsAttributeDict(doc, 'cvsLocation')

# If multiple CVS locations have been specified, translate from a list into
# a space-separated sequence of locations, so we can pass this into the CVS
# checkout command
if (len(cvsLocation) > 1):
	cvsPath = ''
	for pathStr in cvsLocation:
		cvsPath = cvsPath + pathStr + ' '
else:
	cvsPath = cvsLocation[0]

processingMode = "normal"
branchDict = {}

for cvsLocation in cvsLocationsDict:
	entryDict = cvsLocationsDict[cvsLocation]
	
	if 'branchTag' in entryDict.keys():
		processingMode = "branched_module"
		
		branchDict[cvsLocation] = entryDict['branchTag']

# Obtain package build root
pkgBuildRoot = getXMLstringByTagName(doc, 'pkgBuildRoot')

# Obtain package script
pkgReleaseScript = getXMLstringByTagName(doc, 'pkgReleaseScript')

# Obtain package target
pkgTarget = getXMLstringByTagName(doc, 'pkgTarget')

pkgBaseName = getXMLstringByTagName(doc, 'pkgBaseName')

# Obtain package configurations (this entry is optional).  If addDefaultConfigs
# is 'yes' or missing, then we always
# do configurations 'internal' and 'external', so this entry just specifies
# any additional ones.
pkgConfigList = getXMLstringsByTagName(doc, 'pkgConfig')
if pkgConfigList[0] == '':
	pkgConfigList = ['internal', 'external']
	printWhenVerbose('Pkg config list left at default')
elif (addDefaultConfigs == '') or (addDefaultConfigs == 'yes'):
	pkgConfigList.append('internal')
	pkgConfigList.append('external')
	printWhenVerbose('internal and external configurations added to supplied list')
else:
	printWhenVerbose('user-defined configs used')

# Get pre-build and post-build commands (optional)
preBuildCmdElementList = doc.getElementsByTagName('preBuildCommand')
postBuildCmdElementList = doc.getElementsByTagName('postBuildCommand')

# Get top-level directory for unreleased output files (optional)
unreleasedBuildsDir = getXMLstringByTagName(doc, 'unreleasedBuildsDir')

# Get file name for make inc file (optional)
make_incFile = getXMLstringByTagName(doc, 'make_incFile')

# Get publish directory (optional).  If not specified, this will be publishRoot
# as obtained from the command line
publishDir = getXMLstringByTagName(doc, 'publishDir')


#
# Run pre-build commands
#
for element in preBuildCmdElementList:
	cmd = element.childNodes[0].data
	print 'Running pre-build command', cmd
	os.system(cmd)
	# (Don't check return value as we may want to specify commands with a
	# non-zero return value)

#
# Setup top directory path
#
topDir = os.getcwd()
printWhenVerbose('Top directory for checkout ' + topDir)

# 
# REAP (check local module out of CVS)
#
# ... the repository path depends if CVS repository is local or not
if local_cvs:
	pserverString = cvsRoot
else:
	pserverString = ':pserver:' + cvsUserName + '@' + cvsRoot
	
if processingMode == "normal":
	print 'Checking out', cvsPath
	
	cmd = 'cvs -z9 -Q -d ' + pserverString + ' checkout ' + branchTag + ' -P ' + cvsPath
	printWhenVerbose('Executing ' + cmd)
	retCode = os.system(cmd)
	if (retCode != 0):
		print 'ERROR: FAILED CHECKOUT! using:'
		print cmd
		sys.exit(1)

elif processingMode == "branched_module":
	for cvsLocation in cvsLocationsDict.keys():
		localBranchTag = branchTag
		
		if cvsLocation in branchDict.keys():
			localBranchTag = "-r " + branchDict[cvsLocation]
		
		cmd = 'cvs -z9 -Q -d ' + pserverString + ' checkout ' + localBranchTag + ' -P ' + cvsLocation
		printWhenVerbose('Executing ' + cmd)
		retCode = os.system(cmd)
		if (retCode != 0):
			print 'ERROR: FAILED CHECKOUT! using:'
			print cmd
			sys.exit(1)
		
# Obtain build numbers
buildNumFromReleaseNotes = 0 # default
lastBuildNum = doc.documentElement.getAttribute('buildNumber')
externPublish = doc.documentElement.getAttribute('externPublish')
rbuildOutputFlag = doc.documentElement.getAttribute('showOutput')
compiler = doc.documentElement.getAttribute('compiler')

showRbuildOutput = True
if rbuildOutputFlag in ["no", "false"]:
	showRbuildOutput = False

if lastBuildNum == '':
	# Specifying the build number as an attribute in the config file is the
	# old method.  The new method is to obtain it from a "versionised"
	# release note" within the CVS module (the same method used by the
	# RELEASENOTE_VERSIONISE command on the build-and-package system)
	releaseNoteFile = getXMLstringByTagName(doc, 'versionedReleaseNote')
	if (releaseNoteFile == ''):
		print('ERROR: Failed to obtain a version number: release note file not specified')
		sys.exit(1)
	else:
		if os.access(releaseNoteFile, os.F_OK):
			printWhenVerbose('Parsing file ' + releaseNoteFile + ' for build number')
		else:
			print('ERROR: release note file', releaseNoteFile, 'not found')
			sys.exit(1)
		f = open(releaseNoteFile)
		releaseNoteFileContents = ''
		for line in f:
			# This rather impressive regular expression parses the file for a version string
			# and splits it into the requisite 4 fields of the full unique version number
			m = re.match(r"(.*Version:\s+)(?P<major1>\d)(.)(?P<major2>\d+)(.)(?P<major3>\d+)(.)(?P<build>\d+)(.*)", line)
			if m != None:
				majorVer1 = m.group('major1')
				majorVer2 = m.group('major2')
				majorVer3 = m.group('major3')
				lastBuildNum = m.group('build')
				buildNumFromReleaseNotes = 1
				# Increment build number for this build
				thisBuildNum = str(int(lastBuildNum) + 1)
				thisBuildNum = thisBuildNum.rjust(4, '0') # right-justify to width of 4 digits
				# Replace build number in line that will be written back to file
				line = m.group(1) + m.group(2) + m.group(3) + m.group(4) + m.group(5) + m.group(6) + \
				    m.group(7) + thisBuildNum + m.group(9) + '\n'
				printWhenVerbose('New version line for release notes: ' + line)
				
			releaseNoteFileContents = releaseNoteFileContents + line
		f.close()
		lastBuildNum_major = majorVer1 + '.' + majorVer2 + '.' + majorVer3
		lastBuildNum_major_ = majorVer1 + '_' + majorVer2 + '_' + majorVer3
		lastBuildNum_full = lastBuildNum_major + '.' + lastBuildNum
		thisBuildNum_full = lastBuildNum_major + '.' + thisBuildNum
		lastBuildNum_full_ = lastBuildNum_major_ + '_' + lastBuildNum
		thisBuildNum_full_ = lastBuildNum_major_ + '_' + thisBuildNum
else:
	# Old build numbering system
	# Increment build number for this build
	thisBuildNum = str(int(lastBuildNum) + 1)
	lastBuildNum_full = lastBuildNum
	thisBuildNum_full = thisBuildNum
	lastBuildNum_full_ = lastBuildNum
	thisBuildNum_full_ = thisBuildNum
	# Set these to something, but they shouldn't be used
	lastBuildNum_major = 'build'
	lastBuildNum_major_ = 'build'

if (lastBuildNum == ''):
	print('ERROR: Failed to parse release notes for build number')
	sys.exit(1)
	
print 'Last Build:', lastBuildNum
print 'This Build:', thisBuildNum

#
# Determine difference since last build.  If there are no differences, stop here.
#
if (int(lastBuildNum) == 0):
	print 'Building for the first time...'
else:
	print 'Generate change log ...'
	
	# Note we previously used a log command instead of diff here.  However this
	# suffered from 2 problems a) not being able to diff a tag against the head
	# of a branch, and b) not picking up when the only change is to add a file.
	prevLogString = 'RBUILD_' + primaryPackageName +'_' + lastBuildNum_full_
	
	if processingMode == "normal":
		cmd = 'cvs -Q -d ' + pserverString + ' diff -r' + prevLogString \
			+ ' ' + cvsPath + ' > change_log.txt 2>& 1'
		printWhenVerbose('Executing ' + cmd)
		os.system(cmd)
	
	elif processingMode == "branched_module":
		for cvsLocation in cvsLocationsDict:
			cmd = 'cvs -Q -d ' + pserverString + ' diff -r' + prevLogString \
				+ ' ' + cvsLocation + ' >> change_log.txt 2>& 1'
			os.system(cmd)
	
	f = open('change_log.txt', 'r')
	changes_head = f.read(2)
	f.close()
	
	if (changes_head == ""):
		print 'No source changes since last build.'
		# If a build with the expected last build name doesn't exist at the publish location,
		# we assume this is because the toolkit has changed; so re-build even if no source changes.
		expectedPrevOutFile = publishRoot + '/' + primaryPackageName + '_internal_' \
		       + toolKit + '_build_' + lastBuildNum_full + '.tar.gz'
		printWhenVerbose('looking for previous build output file ' + expectedPrevOutFile)
		if os.access(expectedPrevOutFile, os.F_OK):
			print 'The toolkit has not changed.'
			print 'No need to continue'
			sys.exit(0)
		elif (unreleasedBuildsDir != ''):
			# An "unreleased" builds area was specified so check this also, looking for
			# the last build.  Note that in this area each build is expected to be found
			# in its own named directory.
			expectedPrevOutFile = publishRoot + '/' + unreleasedBuildsDir + '/' + lastBuildNum_full \
			   + '/' + primaryPackageName + '_internal_' \
		       + toolKit + '_build_' + lastBuildNum_full + '.tar.gz'
			printWhenVerbose('looking for previous build output file ' + expectedPrevOutFile)
			if os.access(expectedPrevOutFile, os.F_OK):
				print 'The toolkit has not changed.'
				print 'No need to continue'
				sys.exit(0)
		print 'Expected last build file not found:'
		print '   assuming the toolkit or major version number has changed...'
		print 'Building with new toolkit', toolKit
	else:
		print 'Changes since last build.'

if (rbuildMode == 'info'):
	print 'Info only mode finished.'
	sys.exit(0)


if (rbuildMode == 'publish'):
	#
	# Update build number in the file that stores it
	#
	if (buildNumFromReleaseNotes):
		# Build number in release notes file.  We have the updated contents ready from
		# previous step
		f = open(releaseNoteFile, 'w')
		f.write(releaseNoteFileContents)
		f.close()
		# Check back into CVS
		retCode = os.system('cvs commit -m\"Auto-update\" ' + releaseNoteFile)
		if (retCode != 0):
			print('ERROR: CVS commit of release notes failed')
			sys.exit(1)
	else:
		# Build number in package config file
		sed_cmd = 's/buildNumber=\\\"' + lastBuildNum + '\\\"/buildNumber=\\\"' + thisBuildNum + '\\\"/g'
		retCode = os.system('chmod u+w ' + rBuildConfigXMLfile)
		if (retCode != 0):
			print 'ERROR: unable to modify permissions on', rBuildConfigXMLfile
			sys.exit(1)
		retCode = os.system('sed -i ' + sed_cmd + ' ' + rBuildConfigXMLfile)
		if (retCode != 0):
			print 'ERROR: Failed to update build number in config file'
			sys.exit(1)
		retCode = os.system('mv -f ' + rBuildConfigXMLfile + ' ' + publishRoot)
		if (retCode != 0):
			print 'ERROR: Failed moving config file back to publish dir'
			sys.exit(1)
		retCode = os.system('chmod a-w ' + publishRoot + '/'+ rBuildConfigXMLfile)
		if (retCode != 0):
			print 'ERROR: unable to modify permissions on', rBuildConfigXMLfile, 'at publish location'
			sys.exit(1)
	
	#
	# If we have obtained a version number from a release notes file, there is an option
	# to write it out to an include file to be picked up by make
	#
	if (buildNumFromReleaseNotes):
		if (make_incFile != ''):
			print 'Writing make inc file', make_incFile
			f = open(make_incFile, 'w')
			f.write('# Auto-genererated by rbuild: DO NOT EDIT\n')
			# Note turning strings into ints and back again removes leading
			# zeros, which makes it easier to use the values as numbers in code
			f.write('VER_MAJ1=' + str(int(majorVer1)) + '\n')
			f.write('VER_MAJ2=' + str(int(majorVer2)) + '\n')
			f.write('VER_MAJ3=' + str(int(majorVer3)) + '\n')
			f.write('VER_BUILD=' + str(int(thisBuildNum)) + '\n')
			f.close()
			cmd = 'cvs commit -m\"Auto-update\" ' + make_incFile
			retCode = os.system(cmd)
			if (retCode != 0):
				print 'ERROR committing modified make inc file'
				sys.exit(1)

#
# Process package.xml and fetch pre-requisite packages
#
print 'Looking for package.xml'
foundPackageXML = 0

pkgXMLwithPath = pkgBuildRoot + '/package.xml'
if os.access(pkgXMLwithPath, os.F_OK):
	print 'Found package.xml in', pkgBuildRoot
	foundPackageXML = 1
	
	print 'Obtain prerequisites ....'
	getPreRequisites(cvsUserName, publishRoot, pkgBuildRoot)
	
else:
	# Note it is valid for package.xml not to exist; this just means there are no
	# pre-requisites
	print pkgXMLwithPath, 'doesn\'t exist'


#
# Execute package release script 
#
releaseScriptName = os.path.basename(pkgReleaseScript)
releaseScriptDir = os.path.dirname(pkgReleaseScript)
printWhenVerbose('Release script ' + releaseScriptName + ' in directory ' + releaseScriptDir)

# If specified the name of the package passed to release script may be different than the package name itself
# The resulting package archive however will have the name specified by the primaryPackageName
scriptPkgName = primaryPackageName

if pkgBaseName:
	scriptPkgName = pkgBaseName
    
compilerParamString = ""
if compiler:
    compilerParamString = " " + compiler + " "

if os.access(pkgReleaseScript, os.F_OK):
	# Run release script from its own directory
	for pkgConfig in pkgConfigList:
		print 'Executing package release script,', pkgConfig, 'configuration'
		
		if showRbuildOutput == True:
			cmd = 'set -e; set -o pipefail; cd ' + releaseScriptDir + '; chmod +x ' + releaseScriptName + '; ./' \
                  + releaseScriptName + ' ' + scriptPkgName + ' ' + pkgConfig + ' ' + pkgTarget + compilerParamString \
                  + ' | tee ' + topDir + '/package_script_' + pkgConfig + '_log.txt 2>& 1; cd ' + topDir

		else:
			cmd = 'set -e; cd ' + releaseScriptDir + '; chmod +x ' + releaseScriptName + '; ./' \
                  + releaseScriptName + ' ' + scriptPkgName + ' ' + pkgConfig + ' ' + pkgTarget + compilerParamString \
                  + ' > '+ topDir + '/package_script_' + pkgConfig + '_log.txt 2>& 1; cd ' + topDir
		      
		
		printWhenVerbose('Running command: ' + cmd)
		retCode = os.system(cmd)
		if (retCode != 0):
			print 'ERROR occured within package script.'
			sys.exit(1)
else:
	print 'ERROR:', pkgReleaseScript, 'doesn\'t exist'
	sys.exit(1)

#
# Move tars to top dir
#
print 'Copying tars....'
retCode = 0

if pkgBaseName:
	scriptPkgName = pkgBaseName
	files = os.listdir(pkgBuildRoot)
	tarFiles = [f for f in files if f.find('tar.gz') != -1]
	
	for tarFile in tarFiles:
		cmd = 'cp -f ' + pkgBuildRoot + '/' + tarFile + " " + topDir + "/" + tarFile.replace(pkgBaseName, primaryPackageName)
		print cmd
		retCode = os.system(cmd)
		
		print "Ret code:", retCode
		
		if retCode:
			break
	
else:
	cmd = 'cp -f ' + pkgBuildRoot + '/*.tar.gz ' + topDir
	print cmd
	retCode = os.system(cmd)

if (retCode != 0):
	print 'ERROR occured copying tars.'
	#sys.exit(1)

#
# tar up logs.  Note this will also remove change_log.txt
#
print 'tar up logs ...'
buildLog_tarName = primaryPackageName + '_log.tar.gz'
cmd = 'rm -f ' + buildLog_tarName + '; tar -zcvf ' + buildLog_tarName + ' *log.txt; rm -f *log.txt'
retCode = os.system(cmd)
if (retCode != 0):
	print 'ERROR occured zipping up log files'
	sys.exit(1)

#
# Add build number to tars
#
print 'Adding build number to tars ....'
for fileName in os.listdir('.'):
	if fnmatch.fnmatch(fileName, primaryPackageName + '*.tar.gz'):
		if  fnmatch.fnmatch(fileName, '*_build_*'):
			print 'tar file', fileName, 'from a previous build found'
		else:
			newFname = fileName[:-7] + '_build_' + thisBuildNum_full + '.tar.gz'
			printWhenVerbose('New filename ' + newFname)
			cmd = 'mv -f ' + fileName + ' ' + newFname
			retCode = os.system(cmd)
			if (retCode != 0):
				print 'ERROR renaming tar files'
				sys.exit(1)

#
# Run post-build commands
#
for element in postBuildCmdElementList:
	cmd = element.childNodes[0].data
	print 'Running post-build command', cmd
	os.system(cmd)
	# (Don't check return value as we may want to specify commands with a
	# non-zero return value)


#
# Only tag if rbuildMode is publish
#
if (rbuildMode == 'publish'):	
	#
	# TAG
	#
	CVS_tag = 'RBUILD_' + primaryPackageName + '_' + thisBuildNum_full_
	
	if processingMode == "normal":
		print 'Tagging', cvsPath, 'with', CVS_tag
		cmd = 'cvs -Q -d ' + pserverString + ' tag ' + CVS_tag + ' ' + cvsPath
		printWhenVerbose('Running cmd ' + cmd)
		retCode = os.system(cmd)
		if (retCode != 0):
			print 'ERROR performing tagging'
			sys.exit(1)
	
	elif processingMode == "branched_module":
		for cvsLocation in cvsLocationsDict:
			print 'Tagging', cvsLocation, 'with', CVS_tag
			cmd = 'cvs -Q -d ' + pserverString + ' tag ' + CVS_tag + ' ' + cvsLocation
			printWhenVerbose('Running cmd ' + cmd)
			retCode = os.system(cmd)
			if (retCode != 0):
				print 'ERROR performing tagging'
				sys.exit(1)
	
	#
	# Publish
	#
	
	if externPublish not in ['yes', 'true']:
		for fileName in os.listdir('.'):
			if fnmatch.fnmatch(fileName, primaryPackageName + '*.tar.gz'):
				if (publishDir == ''):
					publishDir = publishRoot
				print 'Publishing', fileName
				cmd = 'mkdir -p ' + publishDir +'; chmod -w ' + fileName + \
					'; cp -f ' + fileName + ' ' + publishDir
				printWhenVerbose('Running command ' + cmd)
				retCode = os.system(cmd)
				if (retCode != 0):
					print 'ERROR copying files'
					sys.exit(1)

print('rbuild done')
