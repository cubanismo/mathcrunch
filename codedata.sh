#!/bin/sh
RAWSIZE=`symval "$1" "$2"`
VALUE=$((0x$RAWSIZE + 8 - (0x$RAWSIZE % 8))) 
if [ $3 = endaddr ]; then
    VALUE=$(($VALUE + 0xf03000))
fi

printf "%x" $VALUE
