#!/bin/sh
if [ -f /usr/bin/mcs ]
    then 
      COMPILER=/usr/bin/mcs
  elif [ -f /opt/local/bin/mcs ]
    then
      COMPILER=/opt/local/bin/mcs
  else
      COMPILER=csc.exe
fi

$COMPILER $*
