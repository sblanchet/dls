#!/usr/bin/perl -w

#----------------------------------------------------------------------------
#
# DLS - Status- und Kontroll-Script für den DLS-Daemon
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
use Getopt::Std;

#----------------------------------------------------------------

$| = 1; # Unbuffered output

#----------------------------------------------------------------

my %opt;
my $dls_dir;
my @table;

&main;

#----------------------------------------------------------------
#
#  main
#
#  Main function. Perform all status checks and
#  write the results to STDOUT.
#
#----------------------------------------------------------------

sub main
{
    # Process command line
    &get_options;
    &status;
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
    my $opt_ret = getopts "d:h", \%opt;

    &print_usage if !$opt_ret or $opt{'h'} or $#ARGV > 0;

    # Determine path for DLS data directory
    if ($opt{'d'}) {
	$dls_dir = $opt{'d'};
    }
    elsif (defined $ENV{'DLS_DIR'}) {
	$dls_dir = $ENV{'DLS_DIR'};
    }
    else {
	$dls_dir = "/vol/dls_data";
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
    $0 =~ /^.*\/([^\/]*)$/; # Determine program name

    print "Call: $1 [OPTIONS]\n";
    print "Options:\n";
    print "        -d [directory]     DLS data directory\n";
    print "        -h                 Show this help\n";
    exit 0;
}

#----------------------------------------------------------------
#
#  status
#
#  Return a status report.
#
#----------------------------------------------------------------

sub status
{
    print "\n--- DLS status report ---------------------\n\n";
    print "For DLS data directory \"$dls_dir\"\n\n";

    # Check the specified DLS data directory
    &check_dls_dir;

    # Output table
    print "\n";
    foreach (@table)
    {
	print "$_\n";
    }
    print "\n";

    exit 0;
}

#----------------------------------------------------------------
#
#  check_dls_dir
#
#  Check if the dlsd parent process is running and
#  then start the exams for the individual
#  job directories.
#
#----------------------------------------------------------------

sub check_dls_dir
{
    my $pid;
    my $mother_running = 0;
    my $pid_file = "$dls_dir/dlsd.pid";

    unless (-d $dls_dir) {
        print "ERROR: Directory \"$dls_dir\" does not exist!\n\n";
	return;
    }

    $mother_running = &check_pid($pid_file);

    if ($mother_running == 0) {
        print "DLS parent process NOT RUNNING!\n";
    }
    else {
        print "DLS parent process runs with PID $mother_running.\n";
    }

    &check_jobs;
}

#----------------------------------------------------------------
#
#  check_pid
#
#  Check if a process with PID is still running.
#  If yes, return the PID, otherwise return 0.
#
#----------------------------------------------------------------

sub check_pid
{
    my ($pid_file) = @_;

    return 0 if ! -r $pid_file;

    my $pid = `cat $pid_file`;
    chomp $pid;

    # The content of the first line must be a number
    unless ($pid =~ /^\d+$/) {
	print "ERROR: \"$pid_file\" is corrupted!\n\n";
	return 0;
    }

    `ps ax | grep -q -E "^ *$pid.*dlsd"`;

    return $pid unless $?;
    return 0;
}

#----------------------------------------------------------------
#
#  check_jobs
#
#  Scroll through the entries of the DLS data directory
#  and start the exam job, if one
#  corresponding subdirectory was found.
#
#----------------------------------------------------------------

sub check_jobs
{
    my ($dir_handle, $dir_entry);

    unless (opendir $dir_handle, $dls_dir) {
	print "\nERROR: Cannot open \"$dls_dir\" !\n";
	return;
    }

    push @table, "| Job-ID | Description           | Status    | Process       |";
    push @table, "|--------|-----------------------|-----------|---------------|";

    while ($dir_entry = readdir $dir_handle) {
	# Directory entry must be of the form jobXXX
	next unless $dir_entry =~ /^job(\d+)$/;

	# Directory entry must be a directory
	next unless -d "$dls_dir/$dir_entry";

	&check_job($1);
    }

    closedir $dir_handle;

    push @table, "|________|_______________________|___________|_______________|" ;
}

#----------------------------------------------------------------
#
#  check_job
#
#  Check the status of a single job
#
#  Parameter: job_id - Job-ID
#
#----------------------------------------------------------------

sub check_job
{
    my ($job_id) = @_;

    my $job_file = "$dls_dir/job$job_id/job.xml";
    my $pid_file = "$dls_dir/job$job_id/dlsd.pid";
    my ($job_xml, $desc, $state, $running, $pid, $proc);
    my ($job_id_6, $desc_21, $state_9, $proc_13);

    $desc = "";
    $state = "";
    $running = 0;

    # job.xml must exist
    unless (-r $job_file) {
	print "\nERROR: \"$dls_dir/job$job_id\" -";
	print " File job.xml does not exist!\n";
    }
    else {
	# Attempt to open job.xml
	unless (open JOB, $job_file) {
	    print "\nERROR: \"$job_file\" - File cannot be opened!\n";
	}
	else {
	    # Read the entire file content
	    while (<JOB>) {$job_xml .= $_;}
	    close JOB;

	    # Search for <description text="XXX"/> ...
	    unless ($job_xml =~ /\<description\s+text\s*=\s*\"(.*)\"\s*\/\>/) {
		print "\nERROR: \"$job_file\" -";
		print " <description> tag not found!\n";
	    }
	    else {
		$desc = $1;
	    }

	    # Search for <state name="XXX"/> ...
	    unless ($job_xml =~ /\<state\s+name\s*=\s*\"(.*)\"\s*\/\>/) {
		print "\nERROR: \"$job_file\" -";
		print " <state> tag not found!\n";
	    }
	    else {
		$state = $1;
	    }
	}
    }

    $running = &check_pid($pid_file);

    # Evaluate information for the output
    $proc = "UNKNOWN";
    $proc = "running" if $running > 0;
    $proc = "NOT RUNNING!" if $running == 0;
    $proc = "" if $state eq "paused";
    $desc = "UNKNOWN" if $desc eq "";
    $state = "UNKNOWN" if $state eq "";

    # Format fields
    $job_id_6 = sprintf "%6d", $job_id;
    $desc_21 = sprintf "%21s", $desc;
    $state_9 = sprintf "%9s", $state;
    $proc_13 = sprintf "%13s", $proc;

    # Output fields in table row
    push @table, "| $job_id_6 | $desc_21 | $state_9 | $proc_13 |";
}

#----------------------------------------------------------------
