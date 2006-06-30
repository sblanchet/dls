#!/usr/bin/perl -w

#----------------------------------------------------------------
#
#  DLS - Status- und Kontroll-Script für den DLS-Daemon
#
#  $Id$
#
#----------------------------------------------------------------

use strict;
use FileHandle;
use Getopt::Std;

#----------------------------------------------------------------

$| = 1; # Ungepufferte Ausgabe

#----------------------------------------------------------------

my %opt;
my $dls_dir;
my @table;

my $sleep_after_start = 1;

&main;

#----------------------------------------------------------------
#
#  main
#
#  Hauptfunktion. Führt alle Status-Prüfungen durch und
#  schreibt die Ergebnisse nach STDOUT.
#
#----------------------------------------------------------------

sub main
{
    # Kommandozeile verarbeiten
    &get_options;

    # Kommando ausführen
    &status if not defined $ARGV[0] or $ARGV[0] eq "status";
    &dlsd_start if $ARGV[0] eq "start";
    &dlsd_stop if $ARGV[0] eq "stop";
    &dlsd_restart if $ARGV[0] eq "restart";

    print "FEHLER: Unbekanntes Kommando \"$ARGV[0]\"!\n";
    &print_usage;
}

#----------------------------------------------------------------
#
#  get_options
#
#  Verarbeitet die Kommandozeilenparameter
#
#----------------------------------------------------------------

sub get_options
{
    my $opt_ret = getopts "d:kh", \%opt;

    &print_usage if !$opt_ret or $opt{'h'} or $#ARGV > 0;

    # Pfad für DLS-Datenverzeichnis ermitteln
    if ($opt{'d'})
    {
	$dls_dir = $opt{'d'};
    }
    elsif (defined $ENV{'DLS_DIR'})
    {
	$dls_dir = $ENV{'DLS_DIR'};
    }
    else
    {
	$dls_dir = "/vol/dls_data";
    }
}

#----------------------------------------------------------------
#
#  print_usage
#
#  Gibt die Hilfe über die Kommandozeilenparameter aus
#  und beendet danach den Prozess.
#
#----------------------------------------------------------------

sub print_usage
{
    $0 =~ /^.*\/([^\/]*)$/; # Programmnamen ermitteln

    print "Aufruf: $1 [OPTIONEN] [KOMMANDO]\n";
    print "Optionen:\n";
    print "        -d [Verzeichnis]   DLS-Datenverzeichnis\n";
    print "        -h                 Diese Hilfe anzeigen\n";
    print "Kommandos:\n";
    print "        status             Status anzeigen (Default)\n";
    print "        start              dlsd starten\n";
    print "        stop               dlsd anhalten\n";
    print "        restart            dlsd neu starten\n";
    exit 0;
}

#----------------------------------------------------------------
#
#  status
#
#  Gibt einen Statusbericht aus.
#
#----------------------------------------------------------------

sub status
{
    print "\n--- DLS Statusbericht ---------------------\n\n";
    print "Für DLS-Datenverzeichnis \"$dls_dir\"\n\n";

    # Das angegebene DLS-Datenverzeichnis überprüfen
    &check_dls_dir;

    # Tabelle ausgeben
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
#  dlsd_start
#
#  Startet den dlsd
#
#----------------------------------------------------------------

sub dlsd_start
{
    my $pid_file = "$dls_dir/dlsd.pid";
    my $pid;

    print "Starte dlsd im Verzeichnis \"$dls_dir\"...\n";

    if (-e $pid_file)
    {
	open PID, $pid_file or die "could not open $pid_file";
	$pid = <PID>;
	close PID;

	chomp $pid;

	die "$pid_file is corrupt" unless $pid =~ /\d+/;

	if (kill 0, $pid)
	{
	    print "FEHLER! Es läuft bereits ein dlsd in $dls_dir! PID: $pid\n";
	    exit 1;
	}
    }

    $pid = fork;
    die "could not fork" unless defined $pid;

    if ($pid == 0)
    {
	exec "dlsd", "-d", $dls_dir;
	exit 1;
    }

    # Auf dlsd-Ausgabe warten, damit Prompt nicht überschrieben wird
    sleep $sleep_after_start;

    exit 0;
}

#----------------------------------------------------------------
#
#  dlsd_stop
#
#  Hält den dlsd an
#
#----------------------------------------------------------------

sub dlsd_stop
{
    my $pid_file = "$dls_dir/dlsd.pid";
    my $pid;
    my ($do_not_exit) = @_;

    print "Stoppe dlsd im Verzeichnis \"$dls_dir\"...\n";

    unless (-e $pid_file)
    {
	print "FEHLER: Es läuft kein dlsd im Verzeichnis \"$dls_dir\"!\n";
	exit 1;
    }

    open PID, $pid_file or die "could not open $pid_file";
    $pid = <PID>;
    close PID;

    chomp $pid;

    die "$pid_file is corrupt" unless $pid =~ /\d+/;

    unless (kill 15, $pid)
    {
	print "FEHLER! Konnte Prozess $pid nicht signalisieren!\n";
	exit 1;
    }

    # Warten, bis dlsd beendet wurde
    sleep 1 while (-e $pid_file);

    print "dlsd wurde angehalten.\n";

    exit 0 unless $do_not_exit;
}

#----------------------------------------------------------------
#
#  dlsd_restart
#
#  Startet den dlsd neu
#
#----------------------------------------------------------------

sub dlsd_restart
{
    &dlsd_stop("do_not_exit");
    &dlsd_start;
}

#----------------------------------------------------------------
#
#  check_dls_dir
#
#  Überprüft, ob der dlsd-Mutterprozess läuft und
#  startet dann die Prüfungen für die einzelnen
#  Job-Verzeichnisse.
#
#----------------------------------------------------------------

sub check_dls_dir
{
    my $pid;
    my $mother_running = 0;
    my $pid_file = "$dls_dir/dlsd.pid";

    unless (-e $dls_dir)
    {
	print "FEHLER: Verzeichnis \"$dls_dir\" existiert nicht!\n\n";
	return;
    }

    unless (-d $dls_dir)
    {
	print "FEHLER: \"$dls_dir\" ist kein Verzeichnis!\n\n";
	return;
    }

    if (-e $pid_file)
    {
	# Erste Zeile der PID-Datei einlesen
	unless (open PID, $pid_file)
	{
	    print "FEHLER: Konnte \"$pid_file\" nicht öffnen!\n\n";
	}
	else
	{
	    # PID aus der ersten Zeile der Datei lesen
	    $pid = <PID>;
	    close PID;

	    # Newline abschneiden
	    chomp $pid;

	    # Der Inhalt der ersten Zeile muss eine Nummer sein
	    unless ($pid =~ /^\d+$/)
	    {
		print "FEHLER: \"$pid_file\" ist korrupt!\n\n";
	    }
	    else
	    {
		$mother_running = 1 if kill 0, $pid;
	    }
	}
    }

    if ($mother_running == 0)
    {
	print "DLS-Mutterprozess LÄUFT NICHT!\n";
    }
    else
    {
	print "DLS-Mutterprozess läuft mit PID $pid.\n";
    }

    &check_jobs;
}

#----------------------------------------------------------------
#
#  check_jobs
#
#  Durchläuft die Einträge des DLS-Datenverzeichnisses
#  und startet die Prüfung eines Jobs, wenn ein
#  entsprechendes Unterverzeichnis gefunden wurde.
#
#----------------------------------------------------------------

sub check_jobs
{
    my ($dir_handle, $dir_entry);

    unless (opendir $dir_handle, $dls_dir)
    {
	print "\nFEHLER: Konnte \"$dls_dir\" nicht öffnen!\n";
	return;
    }

    push @table, "| Job-ID | Beschreibung          | Status    | Prozess      |";
    push @table, "|--------|-----------------------|-----------|--------------|";

    while ($dir_entry = readdir $dir_handle)
    {
	# Verzeichniseintrag muss von der Form jobXXX sein
	next unless $dir_entry =~ /^job(\d+)$/;

	# Verzeichniseintrag muss ein Verzeichnis sein
	next unless -d "$dls_dir/$dir_entry";

	&check_job($1);
    }

    closedir $dir_handle;

    push @table, "|________|_______________________|___________|______________|" ;
}

#----------------------------------------------------------------
#
#  check_job
#
#  Überprüft den Status eines einzelnen Jobs
#
#  Parameter: job_id - Auftrags-ID
#
#----------------------------------------------------------------

sub check_job
{
    my ($job_id) = @_;

    my $job_file = "$dls_dir/job$job_id/job.xml";
    my $pid_file = "$dls_dir/job$job_id/dlsd.pid";
    my ($job_xml, $desc, $state, $running, $pid, $proc);
    my ($job_id_6, $desc_21, $state_9, $proc_12);

    $desc = "";
    $state = "";
    $running = 0;

    # job.xml muss existieren
    unless (-e $job_file)
    {
	print "\nFEHLER: \"$dls_dir/job$job_id\" - Datei job.xml existiert nicht!\n";
    }
    else
    {
	# Versuch, job.xml zu öffnen
	unless (open JOB, $job_file)
	{
	    print "\nFEHLER: \"$job_file\" - Datei lässt sich nicht öffnen!\n";
	}
	else
	{
	    # Den gesamten Dateiinhalt einlesen
	    while (<JOB>) {$job_xml .= $_;}
	    close JOB;

	    # Nach <description text="XXX"/> suchen...
	    unless ($job_xml =~ /\<description\s+text\s*=\s*\"(.*)\"\s*\/\>/)
	    {
		print "\nFEHLER: \"$job_file\" - <description>-Tag nicht gefunden!\n";
	    }
	    else
	    {
		$desc = $1;
	    }

	    # Nach <state name="XXX"/> suchen...
	    unless ($job_xml =~ /\<state\s+name\s*=\s*\"(.*)\"\s*\/\>/)
	    {
		print "\nFEHLER: \"$job_file\" - <state>-Tag nicht gefunden!\n";
	    }
	    else
	    {
		$state = $1;
	    }
	}
    }

    if (-e $pid_file) # Wenn PID-Datei existiert
    {
	unless (open PID, $pid_file)
	{
	    print "\nFEHLER: Konnte PID-Datei \"$pid_file\" nicht öffnen!\n";
	    $running = -1; # unbekannt
	}
	else
	{
	    # PID aus der Datei einlesen
	    $pid = <PID>;
	    close PID;

	    chomp $pid; # Newline entfernen

	    unless ($pid =~ /^(\d+)$/) # PID ist keine Nummer
	    {
		print "\nFEHLER: PID-Datei \"$pid_file\" ist korrupt!\n";
		$running = -1; # unbekannt
	    }
	    else
	    {
		# Signal 0 an PID schicken
		$running = 1 if kill 0, $pid;
	    }
	}
    }

    # Informationen für die Ausgabe auswerten
    $proc = "UNBEKANNT";
    $proc = "läuft" if $running == 1;
    $proc = "LÄUFT NICHT!" if $running == 0;
    $proc = "" if $state eq "paused";
    $desc = "UNBEKANNT" if $desc eq "";
    $state = "UNBEKANNT" if $state eq "";

    # Felder formatieren
    $job_id_6 = sprintf "%6d", $job_id;
    $desc_21 = sprintf "%21s", $desc;
    $state_9 = sprintf "%9s", $state;
    $proc_12 = sprintf "%12s", $proc;

    # Felder in Tabellenzeile ausgeben
    push @table, "| $job_id_6 | $desc_21 | $state_9 | $proc_12 |";
}

#----------------------------------------------------------------
