#!/bin/sh
if [ -f /usr/bin/ilasm2 ]
    then
      COMPILER=/usr/bin/ilasm2
  elif [ -f /opt/local/bin/ilasm2 ]
    then
      COMPILER=/opt/local/bin/ilasm2
  else
      COMPILER=ilasm.exe
fi

$COMPILER $*
