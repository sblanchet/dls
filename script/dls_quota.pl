#!/usr/bin/perl -w

#----------------------------------------------------------------------------
#
# DLS - Quota-Script
#
# $Id$
#
# This file is part of the Data Logging Service (DLS).
#
# DLS is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# DLS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with DLS. If not, see <http://www.gnu.org/licenses/>.
#
#----------------------------------------------------------------------------

use strict;
use FileHandle;
use Sys::Syslog;
use Getopt::Std;
use POSIX;

#----------------------------------------------------------------

$| = 1; # Unbuffered output

#----------------------------------------------------------------

my %opt;
my $detached = 0;
my $check_interval = 5; # Seconds
my $dls_dir;
my $progname;

&main;

#----------------------------------------------------------------
#
#  main
#
#  Main function. Evaluate the command line parameters,
#  start the logging, initialize the daemon, install the
#  signal handler and finally run into the main loop
#
#----------------------------------------------------------------

sub main
{
    $0 =~ /^.*\/([^\/]+)$/; # Determine program name
    $progname = $1;

    # Process command line parameters
    &get_options;

    # Syslog initialization
    openlog $progname, "pid";

    &print_and_log("----- DLS QUOTA cleanup started -----");
    &print_and_log("Using DLS directory \"$dls_dir\".");

    if ($check_interval == 0) {
        &print_and_log("Single check mode.");
    } else {
        &print_and_log("Check interval: $check_interval seconds.");
    }

    &print_and_log("Not detaching from tty!") if defined $opt{'k'};

    # Will be a daemon, except if it is forbidden, or single check
    &init_daemon unless (defined $opt{'k'} || $check_interval == 0);

    # Set signal handler
    $SIG{TERM} = $SIG{INT} = \&do_term;

    while (1) {
        # Check all jobs
    	&check_jobs;

        if ($check_interval == 0) {
            &print_and_log("----- DLS QUOTA cleanup exiting -----");
            closelog;
            exit 0;
        }
        # And wait
        sleep $check_interval;
    }
}

#----------------------------------------------------------------
#
#  get_options
#
#  Process the command line parameters
#
#----------------------------------------------------------------

sub get_options
{
    my $optret = getopts "d:i:kh", \%opt;

    &print_usage if defined $opt{'h'} or $#ARGV > -1;

    # Determine path for DLS data directory
    if (defined $opt{'d'}) {
        $dls_dir = $opt{'d'};
    } elsif (defined $ENV{'DLS_DIR'}) {
        $dls_dir = $ENV{'DLS_DIR'};
    } else {
        $dls_dir = "/vol/dls_data";
    }

    if (defined $opt{'i'}) {
        unless ($opt{'i'} =~ /^\d+$/) {
            print "ERROR: the time interval must be an integer!\n";
            &print_usage;
        }

        $check_interval = $opt{'i'};
    }
}

#----------------------------------------------------------------
#
#  print_usage
#
#  Return the help via the command line parameters
#  and then finish the process.
#
#----------------------------------------------------------------

sub print_usage
{
    print "Call: $progname [OPTIONS]\n";
    print "        -d [directory]     DLS data directory\n";
    print "        -i [seconds]       Verification interval (0 = single check)\n";
    print "        -k                 Not a daemon\n";
    print "        -h                 Show this help\n";
    exit 0;
}

#----------------------------------------------------------------
#
#  do_term
#
#  Signal handler for SIGINT and SIGTERM. Give a logging
#  message and then finish the process.
#
#----------------------------------------------------------------

sub do_term
{
    &print_and_log("----- DLS QUOTA cleanup exiting -----");
    closelog;
    exit 0;
}

#----------------------------------------------------------------
#
#  print_and_log
#
#  Generate a logging message that also appears on the STDOUT
#  if no daemon has been initialized
#
#  Parameter: msg - Message
#
#----------------------------------------------------------------

sub print_and_log
{
    my ($msg) = @_;
    print "$msg\n" unless $detached;
    syslog "info", $msg;
}

#----------------------------------------------------------------
#
#  log_and_die
#
#  Create a logging message and leave the process,
#  die after that.
#
#  Parameter: msg - Last words ;-)
#
#----------------------------------------------------------------

sub log_and_die
{
    my ($msg) = @_;
    &print_and_log("ERROR: $msg");
    die $msg;
}

#----------------------------------------------------------------
#
#  init_daemon
#
#  Perform all the necessary actions to create a daemon process
#
#----------------------------------------------------------------

sub init_daemon
{
    # run fork
    my $child = fork;

    &log_and_die("Can't fork: $!") unless defined($child);

    # End parent process
    exit 0 if $child;

    &print_and_log("daemon started with pid $$");

    # Become a session leader
    POSIX::setsid;

    # Change to /
    chdir '/';

    # Close STDXXX
    open STDIN, "</dev/null";
    open STDOUT, ">/dev/null";
    open STDERR, ">&STDOUT";

    $detached = 1;
}

#----------------------------------------------------------------
#
#  check_jobs
#
#  Check all jobs, if they have a quota and
#  lead if necessary the deletion operations.
#
#----------------------------------------------------------------

sub check_jobs
{
    my ($dirhandle, $entry, $path, $job_xml_path);
    my ($quota_time, $quota_size);

    opendir $dirhandle, $dls_dir or &log_and_die("can't open dls directory \"$dls_dir\"!");

    # Scroll through all entries in the DLS data directory
    while ($entry = readdir $dirhandle)
    {
	# Cancel if not "jobXXX"
	next unless $entry =~ /^job\d*$/;

	$path = "$dls_dir/$entry";

	# Cancel if not directory
	next unless -d $path;

	$job_xml_path = "$path/job.xml";

	# Cancel if jobXXX/job.xml does not exist
	next unless -e $job_xml_path;

	$quota_size = 0;
	$quota_time = 0;

	# Get quota information from the XML file
	open JOB_XML, $job_xml_path or &log_and_die("cant open $job_xml_path");
	while (<JOB_XML>)
	{
	    $quota_size = $1 if /\<quota.*size=\"(\d+)\"/;
	    $quota_time = $1 if /\<quota.*time=\"(\d+)\"/;
	}
	close JOB_XML;

	&check_channels_time($path, $quota_time) if $quota_time > 0;
	&check_channels_size($path, $quota_size) if $quota_size > 0;
    }

    closedir $dirhandle;
}

#----------------------------------------------------------------
#
#  check_channels_time
#
#  Check the channel directories for a job with a time quota
#
#  Parameter: job_dir    - Job Directory
#             quota_time - Lenght of time quota in seconds
#
#----------------------------------------------------------------

sub check_channels_time
{
    my ($job_dir, $quota_time) = @_;
    my ($dirhandle, $entry, $path);

    opendir $dirhandle, $job_dir or &log_and_die("can't open job directory \"$job_dir\"!");

    while ($entry = readdir $dirhandle)
    {
	# Cancel if name is not channelXXX
	next unless $entry =~ /^channel\d*$/;

	$path = "$job_dir/$entry";

	# Cancel if no directory
	next unless -d $path;

	&check_chunks_time($path, $quota_time);
    }

    closedir $dirhandle;
}

#----------------------------------------------------------------
#
#  check_chunks_time
#
#  Check the chunck directory for a job with time quota
#
#  Parameter: channel_dir - Channel Directory
#             quota_time  - Length of time quota in seconds
#
#----------------------------------------------------------------

sub check_chunks_time
{
    my ($channel_dir, $quota_time) = @_;
    my ($dirhandle, $entry, $path, @chunk_times, $last_chunk_time);

    # First read all chunk directories
    opendir $dirhandle, $channel_dir or &log_and_die("Can't open channel directory \"$channel_dir\"!");
    while ($entry = readdir $dirhandle)
    {
	# Cancel if name is not chunkXXX
	next unless $entry =~ /^chunk(\d*)$/;

	$path = "$channel_dir/$entry";

	# Cancel if no directory
	next unless -d $path;

	push @chunk_times, $1;
    }
    closedir $dirhandle;

    # Sort chunk times in ascending order
    @chunk_times = sort {$a <=> $b} @chunk_times;

    # Cancel if no chunks found
    return if $#chunk_times == -1;

    # Remember and remove last chunk time
    $last_chunk_time = pop @chunk_times;

    # Go through all the chunk times until the penultimate one
    foreach (@chunk_times)
    {
	# Abort run if the current chunk time
	# is already after the quota limit
	last if $_ >= ($last_chunk_time - $quota_time * 1000000);

	$path = "$channel_dir/chunk$_";

	# Otherwise delete
	&print_and_log("Time quota exceeded - removing $path");
	&remove_dir($path);
    }
}

#----------------------------------------------------------------
#
#  check_channels_size
#
#  Check the total size of a job with data quota
#  and delete the oldest chuncks as needed.
#
#  Parameter: job_dir    - Job Directory
#             quota_size - Size of the data quota in bytes
#
#----------------------------------------------------------------

sub check_channels_size
{
    my ($job_dir, $quota_size) = @_;
    my ($job_dir_handle, $job_dir_entry);
    my ($channel_dir, $channel_dir_handle, $channel_dir_entry);
    my ($chunk_dir, @chunk_times, @chunks, @current_chunks, @du_lines);
    my $total_size;

    opendir $job_dir_handle, $job_dir or &log_and_die("can't open job directory \"$job_dir\"!");
    while ($job_dir_entry = readdir $job_dir_handle)
    {
	# Cancel if name is not channelXXX
	next unless $job_dir_entry =~ /^channel\d*$/;

	$channel_dir = "$job_dir/$job_dir_entry";

	# Cancel if no directory
	next unless -d $channel_dir;

	@chunk_times = ();

	opendir $channel_dir_handle, $channel_dir or &log_and_die("can't open channel directory \"$channel_dir\"!");
	while ($channel_dir_entry = readdir $channel_dir_handle)
	{
	    # Cancel if name is not chunkXXX
	    next unless $channel_dir_entry =~ /^chunk(\d*)$/;

	    $chunk_dir = "$channel_dir/$channel_dir_entry";

	    # Cancel if no directory
	    next unless -d $chunk_dir;

	    push @chunk_times, $1;
	}
	closedir $channel_dir_handle;

	# Continue to the next channel if no chunks are found
	next if $#chunk_times == -1;

	# Sort chunk times in ascending order
	@chunk_times = sort {$a <=> $b} @chunk_times;

	# Memorize the latest chunk separately
	push @current_chunks, "$channel_dir/chunk$chunk_times[$#chunk_times]";

	# Include all chunks in the list
	foreach (@chunk_times)
	{
	    push @chunks, "$channel_dir/chunk$_";
	}
    }
    closedir $job_dir_handle;

    # Call 'du' (disk usage) for each chunk
    @du_lines = ();
    foreach (@chunks)
    {
	push @du_lines, `du -s --block-size=1 $_`; # (Backticks: execute shell command)
    }

    # Sort chunk directories in descending order by timestamp
    @du_lines = sort
    {
	my ($a_time, $b_time);

	$a =~ /.*chunk(\d+)$/ or &log_and_die("sort a");
	$a_time = $1;

	$b =~ /.*chunk(\d+)$/ or &log_and_die("sort b");
	$b_time = $1;

	return $b_time <=> $a_time;
    } @du_lines;

    # Sum total size
    $total_size = 0;
    foreach (@du_lines)
    {
	/(\d+)\s/ or &log_and_die("du parsing error!");
	$total_size += $1;
    }

    # Clear as long as we go below the quota
    chunk: while ($total_size > $quota_size and $#du_lines > -1)
    {
	# Consider the oldest chunk...
	(pop @du_lines) =~ /(\d+)\s+(.*)$/ or &log_and_die("du parsing error!");

	foreach (@current_chunks)
	{
	    # Cancel when the chunk is active
	    next chunk if ($_ eq $2);
	}

	&print_and_log("size quota exceeded - removing $2");
	&remove_dir($2);

	$total_size -= $1;
    }
}

#----------------------------------------------------------------
#
#  remove_dir
#
#  Delete a directory recursively
#
#  Parameter: dir - Directory to delete
#
#----------------------------------------------------------------

sub remove_dir
{
    my ($dir) = @_;
    my ($dirhandle, $entry, $path);

    # Go through all the entries in the directory
    opendir $dirhandle, $dir or &log_and_die("cant open \"$dir\" to remove!");
    while ($entry = readdir $dirhandle)
    {
	# . and .. skip
	next if $entry eq "." or $entry eq "..";

	$path = "$dir/$entry";

	if (-d $path)
	{
	    # Delete subdirectory
	    &remove_dir($path);
	}
	else
	{
	    # Delete file
	    unlink $path;
	}
    }
    closedir $dirhandle;

    # Delete the directory itself
    rmdir $dir;
}

#----------------------------------------------------------------
