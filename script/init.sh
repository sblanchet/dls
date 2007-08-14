#!/bin/sh

#------------------------------------------------------------------------------
#
#  DLS install script
#
#  $Id$
#
#------------------------------------------------------------------------------

### BEGIN INIT INFO
# Provides:          dls
# Required-Start:    $local_fs $syslog $network
# Should-Start:      $time ntp msr etherlab
# Required-Stop:     $local_fs $syslog $network
# Should-Stop:       $time ntp msr etherlab
# Default-Start:     3 5
# Default-Stop:      0 1 2 6
# Short-Description: Data Logging Service
# Description:
### END INIT INFO

#------------------------------------------------------------------------------

SYSCONFIG=/etc/sysconfig/dls

test -r $SYSCONFIG || { echo "$SYSCONFIG not existing";
        if [ "$1" = "stop" ]; then exit 0;
        else exit 6; fi; }

. $SYSCONFIG

#------------------------------------------------------------------------------

if [ -n "$DLS_DIR" ]; then
    PDIR="-d $DLS_DIR"
fi

if [ -n "$DLS_USER" ]; then
    PUSER="-u $DLS_USER"
fi

if [ -n "$DLS_FILES" ]; then
    PFILES="-n $DLS_FILES"
fi

#------------------------------------------------------------------------------

DLSD=dlsd
PIDFILE=$DLS_DIR/dlsd.pid

#------------------------------------------------------------------------------

checkpid() {
    if [ ! -r $PIDFILE ]; then
	/bin/false
	rc_status -v
	rc_exit
    fi

    PID=`cat $PIDFILE`

    if ! kill -0 $PID 2>/dev/null; then
	/bin/false
	rc_status -v
	rc_exit
    fi
}

#------------------------------------------------------------------------------

. /etc/rc.status
rc_reset

case "$1" in
    start)
	echo -n "Starting DLS Daemon "

	if ! $DLSD $PDIR $PUSER $PFILES > /dev/null; then
	    /bin/false
	    rc_status -v
	    rc_exit
	fi

	sleep 1
	checkpid

	rc_status -v
	;;

    stop)
	echo -n "Shutting down DLS Daemon "

	if [ -r $PIDFILE ]; then
	    PID=`cat $PIDFILE`

	    if ! kill -TERM $PID 2>/dev/null; then
		/bin/false
		rc_status -v
		rc_exit
	    fi
	fi

	rc_status -v
	;;

    restart)
	$0 stop || exit 1
	$0 start

	rc_status
	;;

    status)
	echo -n "Checking for DLS Daemon "

	checkpid

	rc_status -v
	;;

    *)
	echo "USAGE: $0 {start|stop|restart|status}"
	;;
esac
rc_exit

#------------------------------------------------------------------------------
