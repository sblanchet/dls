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

#------------------------------------------------------------------------------

DLSD=dlsd
PIDFILE=$DLS_DIR/dlsd.pid

#------------------------------------------------------------------------------

function exit_success()
{
    if [ -r /etc/rc.status ]; then
        rc_reset
        rc_status -v
        rc_exit
    else
        echo " done"
        exit 0
    fi
}

#------------------------------------------------------------------------------

function exit_running()
{
    if [ -r /etc/rc.status ]; then
        rc_reset
        rc_status -v
        rc_exit
    else
        echo " running"
        exit 0
    fi
}

#------------------------------------------------------------------------------

function exit_fail()
{
    if [ -r /etc/rc.status ]; then
        rc_failed
        rc_status -v
        rc_exit
    else
        echo " failed"
        exit 1
    fi
}

#------------------------------------------------------------------------------

function exit_dead()
{
    if [ -r /etc/rc.status ]; then
        rc_failed
        rc_status -v
        rc_exit
    else
        echo " dead"
        exit 1
    fi
}

#------------------------------------------------------------------------------

function checkpid() {
    if [ ! -r $PIDFILE ]; then
        exit_dead
    fi

    PID=`cat $PIDFILE`

    if ! kill -0 $PID 2>/dev/null; then
        exit_dead
    fi
}

#------------------------------------------------------------------------------

if [ -r /etc/rc.status ]; then
    . /etc/rc.status
    rc_reset
fi

case "$1" in
    start)
	echo -n "Starting DLS Daemon "

	if ! $DLSD $PDIR $DLSD_OPTIONS > /dev/null; then
        exit_fail
	fi

	sleep 1
	checkpid
    exit_success
	;;

    stop)
	echo -n "Shutting down DLS Daemon "

	if [ -r $PIDFILE ]; then
	    PID=`cat $PIDFILE`

	    if ! kill -TERM $PID 2>/dev/null; then
            exit_fail
	    fi
	fi

    exit_success
	;;

    restart)
	$0 stop || exit 1
	$0 start
	;;

    status)
	echo -n "Checking for DLS Daemon "

	checkpid
    exit_running
	;;

    *)
	echo "USAGE: $0 {start|stop|restart|status}"
	;;
esac

if [ -r /etc/rc.status ]; then
    rc_exit
else
    exit 1
fi

#------------------------------------------------------------------------------
