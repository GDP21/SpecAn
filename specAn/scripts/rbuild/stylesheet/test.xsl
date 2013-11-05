<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
    <xsl:output method="text" omit-xml-declaration="yes" indent="no"/>
    
<xsl:template match="/">
#! /bin/bash
    <xsl:for-each select="rbuild/rbuildConfig/source">
        <xsl:apply-templates select="@description"/>       
        <xsl:apply-templates select="sourceCVS"/>       
    </xsl:for-each> 
</xsl:template>

    
<xsl:template match="@description">
echo "<xsl:value-of select="."/>"</xsl:template>
    
<xsl:template match="sourceCVS" xml:space="preserve">
<xsl:if test="destination/destinationPath">
if [ ! -e <xsl:value-of select="destination/destinationPath"/> ]; then
mkdir -p <xsl:value-of select="destination/destinationPath"/>;
fi
pushd .
cd <xsl:value-of select="destination/destinationPath"/>
</xsl:if >
cvs -Q -d :pserver:$CVS_USERNAME@<xsl:value-of select="cvsRoot"/> checkout <xsl:if test="cvsTag" >-r <xsl:value-of select="cvsTag"/></xsl:if> <xsl:if test="destination/destinationPath" >-d <xsl:value-of select="destination/destinationName"/> </xsl:if><xsl:value-of select="cvsLocation"/>
<xsl:if test="destination/destinationPath">popd</xsl:if>
</xsl:template>
    
    
</xsl:stylesheet>
