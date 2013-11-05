#! /usr/bin/env python

import os
import sys
import xml.dom.minidom
import re
import string
import shutil
import tempfile
import fnmatch

from rbuild_support import getXMLstringByTagName, getAttributeOfSingleXMLelement, printWhenVerbose, getXMLstringsByTagName

#######
### Test input parameters.

minParams = 4
maxParams = 5

numParams = len(sys.argv) - 1

if (numParams < minParams) or (numParams > maxParams) or (sys.argv[1] == '--help'):
	print """
This script releases a package.  Normally the package being released will have been
created using rbuild.py on the build-and-package system.  The release process consists 
of copying the package to a release area, deleting intermediate builds since the last
release, and removing CVS tags for those unwanted intermediate builds.  There are also
options for "scoring off" and updating a release notes file.

The idea is that other packages must not use a package as a pre-requisite until
the package has been released in this way.

Usage: python release.py [userName] [publishDir] [packageName] [version] [branchTag]

[userName] 		- CVS user name, to be used for CVS check-in and tagging of the release
				notes file.
[publishDir]	- location where the system will pick up its configuration script,
				<packageName>_config.xml.  This is also the destination directory for the release.
[packageName]	- base-name of the package, used to derive the config file name and to
				name the outputs from the build process.
[version]		- full unique version number of the desired release.
[branchTag]		- optional parameter; this specifies a CVS branch tag to use for updating
				the release notes file.  If not present, then CVS HEAD will be assumed

The location of the unreleased build outputs is picked up from the <unreleasedLocation>
element in the file [publishDir]/[packageName]_config.xml.  It is assumed that each
build will be published to a directory named [version] under that location.
Builds with the same version number excluding the current release are assumed to be
unwanted and are removed along with their CVS tags.

"""
	sys.exit(1)

userName = sys.argv[1]
publishDir = sys.argv[2]
packageName = sys.argv[3]
version = sys.argv[4]
if (numParams >= 5 and sys.argv[5] != 'HEAD'):
	branchTag = '-r ' + sys.argv[5]
else:
	branchTag = ''


#
# Parse the config file for relevant info
#
rBuildConfigXMLfile = packageName + '_config.xml'
confFileWithPath = publishDir + '/' + rBuildConfigXMLfile
if os.path.exists(confFileWithPath):
	# parse the file
	doc = xml.dom.minidom.parse(confFileWithPath)
	
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
		print 'Unrecognised local_cvs attribute'
		sys.exit(1)
	
	# Obtain path to CVS module
	cvsLocation = getXMLstringsByTagName(doc, 'cvsLocation')
	# If multiple CVS locations have been specified, translate from a list into
	# a space-separated sequence of locations, so we can pass this into the CVS
	# checkout command
	if (len(cvsLocation) > 1):
		cvsPath = ''
		for pathStr in cvsLocation:
			cvsPath = cvsPath + pathStr + ' '
	else:
		cvsPath = cvsLocation[0]
	
	# Get top-level directory for unreleased output files (optional)
	unreleasedBuildsDir = getXMLstringByTagName(doc, 'unreleasedBuildsDir')

	# Get details of versionised release note (optional)
	releaseNoteFile = getXMLstringByTagName(doc, 'versionedReleaseNote')

	# Get details of release note score-off string (optional)
	releaseNoteScoreOffStr = getXMLstringByTagName(doc, 'releaseNoteScoreOff')
	releaseNoteFormat = getAttributeOfSingleXMLelement(doc, 'releaseNoteScoreOff', 'format')

	# Obtain package configurations (this entry is optional).  We always
	# do configurations 'internal' and 'external', so this entry just specifies
	# any additional ones.
	pkgConfigList = getXMLstringsByTagName(doc, 'pkgConfig')
	if pkgConfigList[0] == '':
		pkgConfigList = ['internal', 'external']
		printWhenVerbose('Pkg config list left at default')
	else:
		pkgConfigList.append('internal')
		pkgConfigList.append('external')
		printWhenVerbose('internal and external configurations added to supplied list')

else:
	print confFileWithPath, 'doesn\'t exist. Can\'t continue.'
	sys.exit(1)

# The repository path depends if CVS repository is local or not
if local_cvs:
	pserverString = cvsRoot
else:
	pserverString = ':pserver:' + userName + '@' + cvsRoot

#
# Check that the requested version number has expected format
#
m = re.match(r"(?P<major1>\d)(.)(?P<major2>\d+)(.)(?P<major3>\d+)(.)(?P<build>\d+)", version)
if (m == None):
	print 'Requested release version doesn\'t have the expected format d.dd.dd.dddd'
	sys.exit(1)
else:
	majorVer1 = m.group('major1')
	majorVer2 = m.group('major2')
	majorVer3 = m.group('major3')
	selectedBuild = m.group('build')


#
# Check that the selected build exists, and that it has not been marked as FAILED by
# the build-and-package system.
#
pkgSourceDir = publishDir + '/' + unreleasedBuildsDir + '/' + version
if os.path.exists(pkgSourceDir):
	if os.path.exists(pkgSourceDir + '/' + 'FAILED'):
		print 'Selected release marked as failed: not continuing'
		sys.exit(1)
	else:
		print 'Releasing build in directory', pkgSourceDir
else:
	print 'Selected release', pkgSourceDir, 'not found'
	sys.exit(1)


#
# Determine a list of unwanted intermediate builds.  This list comprises all the
# builds with the same version number but different build numbers.
#
unwantedBuilds = []
unwantedBuildTags = []
for dirName in os.listdir(publishDir + '/' + unreleasedBuildsDir):
	m = re.match(r"(?P<major1>\d)(.)(?P<major2>\d+)(.)(?P<major3>\d+)(.)(?P<build>\d+)", dirName)
	if m != None:
		if ( (m.group('major1') == majorVer1) and (m.group('major2') == majorVer2) \
		    and (m.group('major3') == majorVer3) and (m.group('build') != selectedBuild) ):
			unwantedBuilds.append(dirName)
			unwantedBuildTags.append('RBUILD_' + packageName + '_' \
			    + majorVer1 + '_' + majorVer2 + '_' + majorVer3 + '_' + m.group('build'))


#
# Delete the intermediate builds and remove their associated build tags in CVS
#
if (len(unwantedBuilds) > 0):
	print 'The following builds and associated CVS tags are scheduled for deletion:'
	for i, unwantedBuild in enumerate(unwantedBuilds):
		print unwantedBuild, '\t', unwantedBuildTags[i]
	
	d = raw_input('Procede with deletions? [Y/n]')
	d = string.lower(d)
	if (d == 'n' or d == 'no'):
		c = raw_input('Deletions cancelled.  Procede with release anyway? [y/N]')
		c = string.lower(c)
		if (c == 'y' or c == 'yes'):
			print 'continuing..'
		else:
			print 'Release cancelled.'
			sys.exit(0)
	else:
		# ... delete unwanted builds
		for dirName in unwantedBuilds:
			fullDirName = publishDir + '/' + unreleasedBuildsDir + '/' + dirName
			print 'deleting', fullDirName, '...'
			shutil.rmtree(fullDirName)
		# ... delete unwanted tags
		for tagName in unwantedBuildTags:
			print 'Removing tag', tagName, '...'
			cmd = 'cvs -Q -d ' + pserverString + ' rtag -d ' + tagName + ' ' + cvsPath
			printWhenVerbose('Running command ' + cmd)
			retCode = os.system(cmd)
			if (retCode != 0):
				print 'FAILED tag deletion using:'
				print cmd
				sys.exit(1)

else:
	print 'No builds are scheduled for deletion.'


#
# Nag the user about release notes
#
rnUpToDate = raw_input('Are the release notes up-to-date? [Y/n]')
rnUpToDate = string.lower(rnUpToDate)
if (rnUpToDate == 'n' or rnUpToDate == 'no'):
	print 'Exiting to give you a chance to update release notes.'
	sys.exit(0)

# Store our current directory so we can get back to it
originalDir = os.getcwd()

#
# "Score off" the release notes file
#
if (releaseNoteScoreOffStr != '' and releaseNoteFile != ''):
	print 'Scoring off release notes..'
	scoreOffRN = 1
	
	# Create temp dir and move into it
	tempdir_rn = tempfile.mkdtemp()
	print 'Working in temp directory', tempdir_rn
	os.chdir(tempdir_rn)
	
	# ... check out release notes
	cmd = 'cvs -z9 -Q -d ' + pserverString + ' checkout ' + branchTag + ' ' + releaseNoteFile
	retCode = os.system(cmd)
	if (retCode != 0):
		print 'FAILED to check out release note file using:'
		print cmd
		sys.exit(1)
	
	# ... score off with version num
	scoreOffLine = releaseNoteScoreOffStr + ' Release ' + version + ' ' + releaseNoteScoreOffStr
	# Add appropriate return tag according to release note format
	if (releaseNoteFormat == 'doxygen'):
		scoreOffLine = scoreOffLine + '\\n'
	elif (releaseNoteFormat == 'html'):
		scoreOffLine = scoreOffLine + '<br>'
	scoreOffLine = scoreOffLine + '\n'
	
	f = open(releaseNoteFile)
	releaseNoteFileContents = ''
	scoreOffLineFound = 0
	for line in f:
		if ((line[:len(releaseNoteScoreOffStr)] == releaseNoteScoreOffStr) and (scoreOffLineFound == 0)):
			releaseNoteFileContents = releaseNoteFileContents + scoreOffLine
			scoreOffLineFound = 1
		releaseNoteFileContents = releaseNoteFileContents + line
	f.close()
	if (scoreOffLineFound):
		# ... write file back with updated contents
		f = open(releaseNoteFile, 'w')
		f.write(releaseNoteFileContents)
		f.close()
	else:
		print 'WARNING: score-off line not found, release notes not modified'

	# ... check back in
	cmd = 'cvs -Q -d ' + pserverString + ' commit -m\"Auto-update for score-off\" ' + releaseNoteFile
	retCode = os.system(cmd)
	if (retCode != 0):
		print 'FAILED to check in release note file using:'
		print cmd
		sys.exit(1)
		
	# ... move tag
	print 'Moving tag on scored-off release notes...'
	CVS_tag = 'RBUILD_' + packageName + '_' + majorVer1 + '_' + majorVer2 + '_' + majorVer3 + '_' + selectedBuild
	cmd = 'cvs -Q -d ' + pserverString + ' tag -F ' + CVS_tag + ' ' + releaseNoteFile
	retCode = os.system(cmd)
	if (retCode != 0):
		print 'FAILED moving tag on release note file, using:'
		print cmd
		sys.exit(1)
	
	# ... (move back to our original directory, in case the user has supplied a
	# relative path for pkgSourceDir)
	os.chdir(originalDir)
	
	#
	# Put updated release note file into each package
	#
	for fileName in os.listdir(pkgSourceDir):
		buildOutputFile = 0    # default
		for config in pkgConfigList:
			if fnmatch.fnmatch(fileName, packageName + '_' + config + '*build_' + version + '.tar.gz'):
				# ... record that we have a match for the fileName
				buildOutputFile = 1
				
				# ... unzip package files for release, in a new temp dir specific to each package
				# ... (do the copy from our original directory, in case the user has supplied a
				# relative path for pkgSourceDir)
				os.chdir(originalDir)
				tempdir = tempfile.mkdtemp()
				print 'Copying and unzipping package', fileName, 'in', tempdir, '...'
				cmd = 'cp ' + pkgSourceDir + '/' + fileName + ' ' + tempdir
				retCode = os.system(cmd)
				if (retCode != 0):
					print 'FAILED copying release package to temp directory using:'
					print cmd
					sys.exit(1)
				cmd = 'tar -xzf ' + tempdir + '/' + fileName + ' -C ' + tempdir
				retCode = os.system(cmd)
				if (retCode != 0):
					print 'FAILED unzipping release package in temp directory using:'
					print cmd
					sys.exit(1)

				# ... check that release note is there in our unzipped package,
				# otherwise something has gone wrong..
				if os.access(tempdir + '/' + releaseNoteFile, os.F_OK):
					# ... replace with updated release note
					cmd = 'cp -f ' + tempdir_rn + '/' + releaseNoteFile + ' ' + tempdir + '/' + releaseNoteFile
					retCode = os.system(cmd)
					if (retCode != 0):
						print 'FAILED copying release note file'
						print cmd
						sys.exit(1)
				else:
					print 'WARNING: release note not found in package where expected'
				
				# ... re-zip package
				os.chdir(tempdir)
				cmd = 'rm -f ' + fileName + '; tar -zcvf ' + fileName + ' *'
				printWhenVerbose('running command ' + cmd)
				retCode = os.system(cmd)
				if (retCode != 0):
					print 'FAILED re-zipping package with command:'
					print cmd
					sys.exit(1)
					
				# ... move back to our original temp dir
				os.chdir(tempdir_rn)

				# ... move package to our other temp directory (current location)
				cmd = 'mv ' + tempdir + '/' + fileName + ' .'
				retCode = os.system(cmd)
				if (retCode != 0):
					print 'FAILED moving package with command:'
					print cmd
					sys.exit(1)

				# ... delete package-specific temp directory
				cmd = 'rm -rf ' + tempdir
				retCode = os.system(cmd)
				if (retCode != 0):
					print 'FAILED deleting temp directory with command:'
					print cmd
					sys.exit(1)
					
		if (buildOutputFile == 0 and fnmatch.fnmatch(fileName, '*.tar.gz')):
			# We have a zip file that is a zip file but not a build output. Assume it is a zipped
			# log file, and copy to our temp location ready for release
			# ... (do the copy from our original directory, in case the user has supplied a
			# relative path for pkgSourceDir)
			os.chdir(originalDir)
			cmd = 'cp ' + pkgSourceDir + '/' + fileName + ' ' + tempdir_rn
			retCode = os.system(cmd)
			if (retCode != 0):
				print 'FAILED copying file with command:'
				print cmd
				sys.exit(1)
else:
	scoreOffRN = 0

#
# Update version number ready to build for next release
#
if (releaseNoteFile != ''):
	# ... ask user if they want to update, and if so what to
	versionUpdate = raw_input('Update version number? [Y/n]')
	versionUpdate = string.lower(versionUpdate)
	if (versionUpdate == 'n' or versionUpdate == 'no'):
		newVersionNum = ''
	else:
		proposedNewVersion = majorVer1 + '.' + majorVer2 + '.' + str(int(majorVer3) + 1)
		newVersionNum = raw_input('New version number [' + proposedNewVersion + ']?')
		if (newVersionNum == ''):
			newVersionNum = proposedNewVersion
		else:
			m = re.match(r"(?P<major1>\d)(.)(?P<major2>\d+)(.)(?P<major3>\d+)", newVersionNum)
			if (m == None):
				print 'WARNING: requested new version string not in expected format d.dd.dd'
				print '... not updating version number'
				newVersionNum = ''

	if (newVersionNum != ''):
		# ... check out release note file in temp directory
		tempdir = tempfile.mkdtemp()
		os.chdir(tempdir)
		cmd = 'cvs -z9 -Q -d ' + pserverString + ' checkout ' + branchTag + ' ' + releaseNoteFile
		retCode = os.system(cmd)
		if (retCode != 0):
			print 'FAILED to check out release note file using:'
			print cmd
			sys.exit(1)
		
		# double-check that release note file is there where we expect it
		if os.access(releaseNoteFile, os.F_OK):
			printWhenVerbose('Parsing file ' + releaseNoteFile + ' for build number')
		else:
			print('ERROR: release note file', releaseNoteFile, 'not found')
			sys.exit(1)
		
		# ... update version number in string version of release notes
		f = open(releaseNoteFile)
		releaseNoteFileContents = ''
		for line in f:
			# This rather impressive regular expression parses the file for a version string
			# and splits it into the requisite 4 fields of the full unique version number
			m = re.match(r"(.*Version:\s+)(?P<major1>\d)(.)(?P<major2>\d+)(.)(?P<major3>\d+)(.)(?P<build>\d+)(.*)", line)
			if m != None:
				# Replace build number in line that will be written back to file
				line = m.group(1) + newVersionNum + '.' + m.group('build') + m.group(9) + '\n'
				printWhenVerbose('New version line for release notes: ' + line)
				
			releaseNoteFileContents = releaseNoteFileContents + line
		f.close()
		
		# ... write back to file
		f = open(releaseNoteFile, 'w')
		f.write(releaseNoteFileContents)
		f.close()

		# ... check file back in
		retCode = os.system('cvs commit -m\"Update version on release\" ' + releaseNoteFile)
		if (retCode != 0):
			print('ERROR: CVS commit of release notes failed')
			sys.exit(1)
		
		# ... move back to original directory, and delete temp directory
		os.chdir(originalDir)
		cmd = 'rm -rf ' + tempdir
		retCode = os.system(cmd)
		if (retCode != 0):
			print 'FAILED deleting temp directory with command:'
			print cmd
			sys.exit(1)

# Change back to original directory if we haven't already done so
os.chdir(originalDir)


#
# Publish
#
# If we did the scoring off of release note file then our packages will be in a temp directory.
# Otherwise, just copy them from their original location.
print 'Publishing released packages...'
if (scoreOffRN):
	cmd = 'mv ' + tempdir_rn + '/*.tar.gz ' + publishDir
else:
	cmd = 'mv ' + pkgSourceDir + '/*.tar.gz ' + publishDir
	
retCode = os.system(cmd)
if (retCode != 0):
	print 'FAILED publishing packages with command:'
	print cmd
	sys.exit(1)

#
# Remove temp directory if necessary
#
if (scoreOffRN):
	cmd = 'rm -rf ' + tempdir_rn
	retCode = os.system(cmd)
	if (retCode != 0):
		print 'FAILED deleting temp directory with command:'
		print cmd
		sys.exit(1)

#
# Option to remove directory in unpublished area
#
rmUnpubl = raw_input('Remove build directory in unpublished location? [Y/n]')
rmUnpubl = string.lower(rmUnpubl)
if (rmUnpubl == 'n' or rmUnpubl == 'no'):
	print 'WARNING: if you don\'t delete this directory, then the next release may'
	print 'cause the tags for this release to be deleted.  It is strongly recommended'
	print 'that you either delete the direcory manually, or move it elsewhere.'
	sys.exit(0)
else:
	cmd = 'rm -rf ' + pkgSourceDir
	retCode = os.system(cmd)
	if (retCode != 0):
		print 'FAILED deleting unpublished build directory with command:'
		print cmd
		sys.exit(1)
	
print 'Release process complete.'

