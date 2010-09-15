#!/bin/sh
#

u=${USER-the_zephyr_builder}
h=`hostname`
t=`date`

/bin/echo "#define ZSERVER_VERSION_STRING \"(${t}) ${u}@${h}\""
