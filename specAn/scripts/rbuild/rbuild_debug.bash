#! /bin/bash

#
# Functions
#
DEBUG_ENABLE="0"
DEBUG_LEVEL="2"

# param1 - debugLevel
# param2 - string
function debug ()
{
    if [ $DEBUG_ENABLE = "1" ]; then
		if [ "$1" -gt $DEBUG_LEVEL ]; then
		echo $2
		fi
    fi
}

# param1 - string
function displayInfo ()
{
    echo $1
}
