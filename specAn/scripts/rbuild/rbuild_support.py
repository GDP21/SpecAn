#! /usr/bin/env python

import xml.dom.minidom

verbose = 0

#######################################################################
def getXMLstringByTagName(element, tagName):
	"""
	Extract a data string from an XML file.
	This function is for use where there is only 1 tag of the required
	type expected to be present in the document: it returns the text
	string associated with the tag (or a null string if the tag is not found).
	element holds parsed file information (produced with xml.dom.minidom.parse).
	It can point to either the top-level doc element, or some sub-element within
	the XML structure.
	tagName is a string holding the desired tag name
	"""
	elementList = element.getElementsByTagName(tagName)
	if (len(elementList) > 0):
		data = elementList[0].childNodes[0].data
		if verbose:
			if data == '':
				print 'info: no data for tag', tagName
			else:
				print 'info: Data for tag', tagName, 'is', data
		return(data)
	else:
		return ''


#######################################################################
def getXMLstringsByTagName(element, tagName):
	"""
	Extract data strings from an XML file.
	This function is for use where 1 or more tags of the required
	type are expected to be present in the document: it returns a list of text
	strings associated with the tag (or a null string if the tag is not found).
	element holds parsed file information (produced with xml.dom.minidom.parse).
	It can point to either the top-level doc element, or some sub-element within
	the XML structure.
	tagName is a string holding the desired tag name
	"""
	elementList = element.getElementsByTagName(tagName)
	numElements = len(elementList)
	if (numElements > 0):
		data = elementList[0].childNodes[0].data
		# Create a list, even if we only have 1 element
		# (so we can use the append() function if required)
		data = [data]
		i = 1
		while (numElements > i):
			data.append(elementList[i].childNodes[0].data)
			i = i + 1
		if verbose:
			if data == '':
				print 'info: no data for tag', tagName
			else:
				print 'info: Data for tag', tagName, 'is', data
		return(data)
	else:
		return ['']

#######################################################################
def getAttributeOfSingleXMLelement(element, tagName, attribute):
	"""
	Extract an attribute value string from an XML file.
	This function is for use where there is only 1 tag of the required
	type expected to be present in the document: it returns the value
	of a specified attribute associated with the tag (or a null string if
	either the tag or the attribute are not found).
	element holds parsed file information (produced with xml.dom.minidom.parse).
	It can point to either the top-level doc element, or some sub-element within
	the XML structure.
	tagName is a string holding the desired tag name
	attribute is a string holding the desired attribute name
	"""
	elementList = element.getElementsByTagName(tagName)
	if (len(elementList) > 0):
		attributeVal = elementList[0].getAttribute(attribute)
		if verbose:
			if attributeVal == '':
				print 'info: No attribute of type', attribute, 'found for tag', tagName
			else:
				print 'info: Attribute', attribute, 'for tag', tagName, 'is', attributeVal
		return(attributeVal)
	else:
		return ''
		
def getElementsAttributeDict(element, tagName):		
	"""
	Extract all attribute name, value pairs for a node with given name.
	As a result each attribute's name will create a key in the element's 
	local dictionary with value being the value stored under the attribute's
	name. Local dictionaries created for each element will be put in a
	common dictionary under the key which is element's text. This dictionary
	is then returned as a result of the execution of this function.
	
	NOTE: in oder for this function to perform correctly, elements need to
	have text data field
	"""
	
	elementList = element.getElementsByTagName(tagName)
	
	elementDict = {}
	
	for element in elementList:
		elementName = element.childNodes[0].data
		
		attributeDict = {}
		for propName, propVal in element.attributes.items():
			attributeDict[propName] = propVal
			
		elementDict[elementName] = attributeDict
		
	return elementDict


#######################################################################
def printWhenVerbose(string):
	if verbose:
		print string



