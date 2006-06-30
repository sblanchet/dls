#!/usr/bin/perl -w

#----------------------------------------------------------------
#
#  DLS - Quota-Script
#
#  $Id$
#
#----------------------------------------------------------------

use strict;
use FileHandle;
use Sys::Syslog;
use Getopt::Std;
use POSIX;

#----------------------------------------------------------------

$| = 1; # Ungepufferte Ausgabe

#----------------------------------------------------------------

my %opt;
my $detached = 0;
my $check_interval = 5; # Sekunden
my $dls_dir;

&main;

#----------------------------------------------------------------
#
#  main
#
#  Hauptfunktion. Wertet die Kommandozeilenparameter aus, startet
#  das Logging, initialisiert den Daemon, installiert die
#  Signalhandler und l�uft schliesslich in die Hauptschleife 
#
#----------------------------------------------------------------

sub main
{
    # Kommandozeilenparameter verarbeiten
    &get_options;

    # Syslog initialisieren
    openlog "dls_cleanup.pl", "pid";

    &print_and_log("----- DLS QUOTA cleanup started -----");
    &print_and_log("Using DLS directory \"$dls_dir\".");
    &print_and_log("Check interval: $check_interval seconds.");
    &print_and_log("Not detaching from tty!") if defined $opt{'k'};

    # Daemon werden, wenn nicht verboten
    &init_daemon unless defined $opt{'k'};

    # Signalhandler setzen
    $SIG{TERM} = $SIG{INT} = \&do_term;

    while (1)
    {
	# Alle Jobs �berpr�fen
	&check_jobs;

	# Und warten
	sleep $check_interval;
    }
}

#----------------------------------------------------------------
#
#  get_options
#
#  Verarbeitet die Kommandozeilenparameter.
#
#----------------------------------------------------------------

sub get_options
{
    my $optret = getopts "d:i:kh", \%opt;

    &print_usage if defined $opt{'h'} or $#ARGV > -1;

    # Pfad f�r DLS-Datenverzeichnis ermitteln
    if (defined $opt{'d'})
    {
	$dls_dir = $opt{'d'};
    }
    elsif (defined $ENV{'DLS_DATA'})
    {
	$dls_dir = $ENV{'DLS_DATA'};
    }
    else
    {
	$dls_dir = "/vol/dls_data";
    }

    if (defined $opt{'i'})
    {
	unless ($opt{'i'} =~ /^\d+$/)
	{
	    print "FEHLER: Zeitintervall muss eine Ganzzahl sein!\n";
	    &print_usage;
	}

	$check_interval = $opt{'i'};
    }
}

#----------------------------------------------------------------
#
#  print_usage
#
#  Gibt die Hilfe �ber die Kommandozeilenparameter aus
#  und beendet danach den Prozess.
#
#----------------------------------------------------------------

sub print_usage
{
    $0 =~ /^.*\/([^\/]+)$/; # Programmnamen ermitteln

    print "Aufruf: $1 [OPTIONEN]\n";
    print "        -d [Verzeichnis]   DLS-Datenverzeichnis\n";
    print "        -i [Sekunden]      �berpr�fungs-Intervall\n";
    print "        -k                 Kein Daemon werden\n";
    print "        -h                 Diese Hilfe anzeigen\n";
    exit 0;
}

#----------------------------------------------------------------
#
#  do_term
#
#  Signalhandler f�r SIGINT und SIGTERM. Gibt eine Logging-
#  Nachricht aus und beendet dann den Prozess.
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
#  Erzeugt eine Logging-Nachricht, die auch auf dem STDOUT
#  erscheint, wenn kein Daemon initialisiert wurde
#
#  Parameter: msg - Nachricht
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
#  Erstellt eine Logging-Nachricht und l�sst den Prozess
#  danach sterben.
#
#  Parameter: msg - Letzte Worte ;-)
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
#  F�hrt alle n�tigen Aktionen aus, um einen Daemon-Prozess
#  zu erzeugen.
#
#----------------------------------------------------------------

sub init_daemon
{
    # Fork ausf�hren
    my $child = fork;

    &log_and_die("Can't fork: $!") unless defined($child);

    # Elternprozess beenden
    exit 0 if $child;

    &print_and_log("daemon started with pid $$");

    # Session leader werden
    POSIX::setsid;

    # Nach / wechseln
    chdir '/';

    # STDXXX schliessen
    open STDIN, "</dev/null";
    open STDOUT, ">/dev/null";
    open STDERR, ">&STDOUT";

    $detached = 1;
}

#----------------------------------------------------------------
#
#  check_jobs
#
#  Pr�ft bei allen Jobs, ob sie eine Quota haben und f�hrt
#  bei Bedarf die n�tigen L�sch-Operationen aus.
#
#----------------------------------------------------------------

sub check_jobs
{
    my ($dirhandle, $entry, $path, $job_xml_path);
    my ($quota_time, $quota_size);

    opendir $dirhandle, $dls_dir or &log_and_die("can't open dls directory \"$dls_dir\"!");

    # Alle Eintr�ge im DLS-Datenverzeichnis durchlaufen
    while ($entry = readdir $dirhandle)
    {
	# Abbrechen, wenn nicht "jobXXX"
	next unless $entry =~ /^job\d*$/;

	$path = "$dls_dir/$entry";

	# Abbrechen, wenn kein Verzeichnis
	next unless -d $path;

	$job_xml_path = "$path/job.xml";

	# Abbrechen, wenn jobXXX/job.xml nicht existiert
	next unless -e $job_xml_path;

	$quota_size = 0;
	$quota_time = 0;

	# Quota-Informationen aus dem XML holen
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
#  �berpr�ft bei einem Job mit Zeit-Quota die einzelnen
#  Kanal-Verzeichnisse.
#
#  Parameter: job_dir    - Job-Verzeichnis
#             quota_time - L�nge der Zeit-Quota in Sekunden
# 
#----------------------------------------------------------------

sub check_channels_time
{
    my ($job_dir, $quota_time) = @_;
    my ($dirhandle, $entry, $path);

    opendir $dirhandle, $job_dir or &log_and_die("can't open job directory \"$job_dir\"!");

    while ($entry = readdir $dirhandle)
    {
	# Abbrechen, wenn Name nicht channelXXX
	next unless $entry =~ /^channel\d*$/;

	$path = "$job_dir/$entry";

	# Abbrechen, wenn kein Verzeichnis
	next unless -d $path;

	&check_chunks_time($path, $quota_time);
    }

    closedir $dirhandle;
}

#----------------------------------------------------------------
#
#  check_chunks_time
#
#  �berpr�ft bei einem Job mit Zeit-Quota die einzelnen Chunk-
#  Verzeichnisse.
#
#  Parameter: channel_dir - Channel-Verzeichnis
#             quota_time  - L�nge der Zeit-Quota in Sekunden
#
#----------------------------------------------------------------

sub check_chunks_time
{
    my ($channel_dir, $quota_time) = @_;
    my ($dirhandle, $entry, $path, @chunk_times, $last_chunk_time);

    # Zuerst alle Chunk-Verzeichnisse einlesen
    opendir $dirhandle, $channel_dir or &log_and_die("Can't open channel directory \"$channel_dir\"!");
    while ($entry = readdir $dirhandle)
    {
	# Abbrechen, wenn Name nicht chunkXXX
	next unless $entry =~ /^chunk(\d*)$/;

	$path = "$channel_dir/$entry";

	# Abbrechen, wenn kein Verzeichnis
	next unless -d $path;

	push @chunk_times, $1;
    }
    closedir $dirhandle;

    # Chunk-Zeiten aufsteigend sortieren
    @chunk_times = sort {$a <=> $b} @chunk_times;

    # Abbrechen, wenn keine Chunks gefunden
    return if $#chunk_times == -1;

    # Letzte Chunk-Zeit merken und entfernen
    $last_chunk_time = pop @chunk_times;

    # Alle Chunk-Zeiten bis zur vorletzten durchlaufen
    foreach (@chunk_times)
    {
	# Durchlauf abbrechen, wenn die aktuelle Chunk-Zeit
	# bereits nach der Quota-grenze liegt
	last if $_ >= ($last_chunk_time - $quota_time * 1000000);

	$path = "$channel_dir/chunk$_";

	# Sonst l�schen
	&print_and_log("Time quota exceeded - removing $path");
	&remove_dir($path);
    }
}

#----------------------------------------------------------------
#
#  check_channels_size
#
#  �berpr�ft bei einem Job mit Daten-Quota die Gesamtgr��e
#  und l�scht bei Bedarf die �ltesten Chunks.
#
#  Parameter: job_dir    - Job-Verzeichnis
#             quota_size - Gr��e der Daten-Quota in Bytes
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
	# Abbrechen, wenn Name nicht channelXXX
	next unless $job_dir_entry =~ /^channel\d*$/;

	$channel_dir = "$job_dir/$job_dir_entry";

	# Abbrechen, wenn kein Verzeichnis
	next unless -d $channel_dir;

	@chunk_times = ();

	opendir $channel_dir_handle, $channel_dir or &log_and_die("can't open channel directory \"$channel_dir\"!");
	while ($channel_dir_entry = readdir $channel_dir_handle)
	{
	    # Abbrechen, wenn Name nicht chunkXXX
	    next unless $channel_dir_entry =~ /^chunk(\d*)$/;

	    $chunk_dir = "$channel_dir/$channel_dir_entry";

	    # Abbrechen, wenn kein Verzeichnis
	    next unless -d $chunk_dir;

	    push @chunk_times, $1;
	}
	closedir $channel_dir_handle;

	# Mit dem n�chsten Channel fortfahren, wenn keine Chunks gefunden
	next if $#chunk_times == -1;

	# Chunk-Zeiten aufsteigend sortieren
	@chunk_times = sort {$a <=> $b} @chunk_times;

	# Den aktuellsten Chunk gesondert merken
	push @current_chunks, "$channel_dir/chunk$chunk_times[$#chunk_times]";

	# Alle Chunks in die Liste aufnehmen
	foreach (@chunk_times)
	{
	    push @chunks, "$channel_dir/chunk$_";
	}
    }
    closedir $job_dir_handle;

    # F�r jeden Chunk ein 'du' aufrufen
    @du_lines = ();
    foreach (@chunks)
    {
	push @du_lines, `du -s --block-size=1 $_`; # (Backticks: Shell-Kommando ausf�hren)
    }

    # Chunk-Verzeichnisse absteigend nach Zeitstempel sortieren
    @du_lines = sort
    {
	my ($a_time, $b_time);

	$a =~ /.*chunk(\d+)$/ or &log_and_die("sort a");
	$a_time = $1;

	$b =~ /.*chunk(\d+)$/ or &log_and_die("sort b");
	$b_time = $1;

	return $b_time <=> $a_time;
    } @du_lines;

    # Gesamtgr��e aufsummieren
    $total_size = 0;
    foreach (@du_lines)
    {
	/(\d+)\s/ or &log_and_die("du parsing error!");
	$total_size += $1;
    }

    # Solange l�schen, bis wir die Quota unterschreiten
    chunk: while ($total_size > $quota_size and $#du_lines > -1)
    {
	# Den �ltesten Chunk betrachten...
	(pop @du_lines) =~ /(\d+)\s+(.*)$/ or &log_and_die("du parsing error!");
	
	foreach (@current_chunks)
	{
	    # Abbrechen, wenn der Chunk aktiv ist
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
#  L�scht ein Verzeichnis rekursiv.
#
#  Parameter: dir - Zu l�schendes Verzeichnis
#
#----------------------------------------------------------------

sub remove_dir
{
    my ($dir) = @_;
    my ($dirhandle, $entry, $path);

    # Alle Eintr�ge des verzeichnisses durchlaufen
    opendir $dirhandle, $dir or &log_and_die("cant open \"$dir\" to remove!");
    while ($entry = readdir $dirhandle)
    {
	# . und .. �berspringen
	next if $entry eq "." or $entry eq "..";

	$path = "$dir/$entry";

	if (-d $path)
	{
	    # Unterverzeichnis l�schen
	    &remove_dir($path);
	}
	else
	{
	    # Datei l�schen
	    unlink $path;
	}
    }
    closedir $dirhandle;

    # Das Verzeichnis selber l�schen
    rmdir $dir;
}

#----------------------------------------------------------------
