#!/bin/sh

rf_start ()
{
    DEV=$1
    ADDR=$(hcitool scan | \
    {
	    ADDR= 
	    while read ADDR NAME
	      do
	      if [ "$NAME" = "G-Rays2" ]  ; then
		  $SUDO rfcomm bind $DEV $ADDR
		  echo $ADDR
		  exit
	      fi
	    done
	    }
    )
    [ -n "$ADDR" ]
}

SUDO=gksudo

DEV=${2:-/dev/rfcomm0}

case $1 in 
start)
  [ -c $DEV ] || rf_start $DEV
  ;;

stop)
  [ -c $DEV ] && $SUDO rfcomm release $DEV
  ;;
esac
