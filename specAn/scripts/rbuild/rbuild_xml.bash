#! /bin/bash

RESULT=""

# param1 - elementString
# param2 - xmlFile
function extractElement ()
{

    EGREP_COMMAND='<('$1')\b[^>]*>.*?</\1>'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep -o $EGREP_COMMAND $2 | egrep -o '>[A-Za-z0-9_.:/-]+<' | egrep -o '[^>^<]+'`
}


# param1 - elementString
# param2 - xmlString
function extractElementFromString ()
{

    EGREP_COMMAND='<('$1')\b[^>]*>.*?</\1>'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`echo $2 | egrep -o $EGREP_COMMAND | egrep -o '>[A-Za-z0-9_.:/]+<' | egrep -o '[^>^<]+'`
}


#
# For each occurance of an element name, 
# extract into an array <elementName>.*?</elementName>
#
# param1 - elementString
# param2 - xmlFile
#
function extractElementContents ()
{

    EGREP_COMMAND='<('$1')\b[^>]*>.*?</\1>'
    
    # debug 1 $EGREP_COMMAND
    
    # Need to strip carriage returns and line feeds

    # Use sed to insert '' around <$1> </$1>	
    
    SED_COMMAND2='s/'$1'>/'$1'>\n/g'

    # Remove line feed characters using tr -d
    # Then using sed add line feed character to closing tag of specified elementName
    # Then grep with regular experession

    cat $2 | tr -d '\r' | tr -d '\n' | sed $SED_COMMAND2 | egrep -o $EGREP_COMMAND > rbuild.tmp

 
    # Now create the array
        
    # create file descripter 3
    exec 3< rbuild.tmp  
   
    unset RESULT
    while read -u 3 -r line; do
    RESULT[${#RESULT[@]}]=$line;
    done;

  
}


# param1 - attributeString
# param2 - xmlFile
function extractIntegerAttribute ()
{

    EGREP_COMMAND=$1'="[0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep -o $EGREP_COMMAND $2 | egrep -o -e'[.0-9]+'`
}

# param1 - attributeString
# param2 - xmlFile
function extractBuildNumberAttribute ()
{

    EGREP_COMMAND=$1'="[.0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep -o $EGREP_COMMAND $2 | egrep -o '[.0-9]+'`
}


# param1 - attributeString
# param2 - xmlFile
function extractBuildNumberShortAttribute ()
{

    EGREP_COMMAND=$1'="[.0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep -o $EGREP_COMMAND $2 | egrep -o '"[.0-9]+"' | egrep -o "[0-9]+\\."`
}

# param1 - attributeString
# param2 - xmlFile
function extractBuildNumberFirstIntegerAttribute ()
{

    EGREP_COMMAND=$1'="[.0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep $EGREP_COMMAND $2 | egrep -o '"'"[0-9]+"'.' | egrep -o "[0-9]+"`
}

# param1 - attributeString
# param2 - xmlFile
function extractBuildNumberFinalIntegerAttribute ()
{

    EGREP_COMMAND=$1'="[.0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep $EGREP_COMMAND $2 | egrep -o '\\.[0-9]+"' | egrep -o [0-9]+`
}


# param1 - attributeString
# param2 - xmlFile
function extractStringAttribute ()
{

    EGREP_COMMAND=$1'="[A-Za-z_0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`egrep -o $EGREP_COMMAND $2 | egrep -o '".*"' | egrep -o '[A-Za-z_0-9]+'`
}

# param1 - attributeString
# param2 - xmlString
function extractStringAttributeFromString ()
{

    EGREP_COMMAND=$1'="[A-Za-z_0-9]+"'
    
#    debug 1 $EGREP_COMMAND
    
    RESULT=`echo $2 | egrep -o $EGREP_COMMAND | egrep -o '".*"' | egrep -o '[A-Za-z_0-9]+'`
}


# 
# End of functions ------------------------------------------------------------
#
