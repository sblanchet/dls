#!/usr/bin/perl -w

#----------------------------------------------------------------
#
#  DLS - Status- und Kontroll-Script f�r den DLS-Daemon
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

&main;

#----------------------------------------------------------------
#
#  main
#
#  Hauptfunktion. F�hrt alle Status-Pr�fungen durch und
#  schreibt die Ergebnisse nach STDOUT.
#
#----------------------------------------------------------------

sub main
{
    # Kommandozeile verarbeiten
    &get_options;
    &status;
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
    my $opt_ret = getopts "d:h", \%opt;

    &print_usage if !$opt_ret or $opt{'h'} or $#ARGV > 0;

    # Pfad f�r DLS-Datenverzeichnis ermitteln
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
#  Gibt die Hilfe �ber die Kommandozeilenparameter aus
#  und beendet danach den Prozess.
#
#----------------------------------------------------------------

sub print_usage
{
    $0 =~ /^.*\/([^\/]*)$/; # Programmnamen ermitteln

    print "Aufruf: $1 [OPTIONEN]\n";
    print "Optionen:\n";
    print "        -d [Verzeichnis]   DLS-Datenverzeichnis\n";
    print "        -h                 Diese Hilfe anzeigen\n";
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
    print "Fuer DLS-Datenverzeichnis \"$dls_dir\"\n\n";

    # Das angegebene DLS-Datenverzeichnis �berpr�fen
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
#  check_dls_dir
#
#  �berpr�ft, ob der dlsd-Mutterprozess l�uft und
#  startet dann die Pr�fungen f�r die einzelnen
#  Job-Verzeichnisse.
#
#----------------------------------------------------------------

sub check_dls_dir
{
    my $pid;
    my $mother_running = 0;
    my $pid_file = "$dls_dir/dlsd.pid";

    unless (-d $dls_dir) {
	print "FEHLER: Verzeichnis \"$dls_dir\" existiert nicht!\n\n";
	return;
    }

    if (-r $pid_file) {
	$pid = `cat $pid_file`;
	chomp $pid;
	
	# Der Inhalt der ersten Zeile muss eine Nummer sein
	unless ($pid =~ /^\d+$/) {
	    print "FEHLER: \"$pid_file\" ist korrupt!\n\n";
	}
	else {
	    `ps ax | grep -q -E "^ +$pid.*dlsd"`;
	    $mother_running = 1 unless $?;
	}
    }

    if ($mother_running == 0) {
	print "DLS-Mutterprozess LAEUFT NICHT!\n";
    }
    else {
	print "DLS-Mutterprozess laeuft mit PID $pid.\n";
    }

    &check_jobs;
}

#----------------------------------------------------------------
#
#  check_jobs
#
#  Durchl�uft die Eintr�ge des DLS-Datenverzeichnisses
#  und startet die Pr�fung eines Jobs, wenn ein
#  entsprechendes Unterverzeichnis gefunden wurde.
#
#----------------------------------------------------------------

sub check_jobs
{
    my ($dir_handle, $dir_entry);

    unless (opendir $dir_handle, $dls_dir) {
	print "\nFEHLER: Konnte \"$dls_dir\" nicht �ffnen!\n";
	return;
    }

    push @table, "| Job-ID | Beschreibung          | Status    | Prozess       |";
    push @table, "|--------|-----------------------|-----------|---------------|";

    while ($dir_entry = readdir $dir_handle) {
	# Verzeichniseintrag muss von der Form jobXXX sein
	next unless $dir_entry =~ /^job(\d+)$/;

	# Verzeichniseintrag muss ein Verzeichnis sein
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
#  �berpr�ft den Status eines einzelnen Jobs
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
    my ($job_id_6, $desc_21, $state_9, $proc_13);

    $desc = "";
    $state = "";
    $running = 0;

    # job.xml muss existieren
    unless (-r $job_file) {
	print "\nFEHLER: \"$dls_dir/job$job_id\" -";
	print " Datei job.xml existiert nicht!\n";
    }
    else {
	# Versuch, job.xml zu �ffnen
	unless (open JOB, $job_file) {
	    print "\nFEHLER: \"$job_file\" - Datei l�sst sich nicht �ffnen!\n";
	}
	else {
	    # Den gesamten Dateiinhalt einlesen
	    while (<JOB>) {$job_xml .= $_;}
	    close JOB;

	    # Nach <description text="XXX"/> suchen...
	    unless ($job_xml =~ /\<description\s+text\s*=\s*\"(.*)\"\s*\/\>/) {
		print "\nFEHLER: \"$job_file\" -";
		print " <description>-Tag nicht gefunden!\n";
	    }
	    else {
		$desc = $1;
	    }

	    # Nach <state name="XXX"/> suchen...
	    unless ($job_xml =~ /\<state\s+name\s*=\s*\"(.*)\"\s*\/\>/) {
		print "\nFEHLER: \"$job_file\" -";
		print " <state>-Tag nicht gefunden!\n";
	    }
	    else {
		$state = $1;
	    }
	}
    }

    if (-r $pid_file) { # Wenn PID-Datei existiert
	$pid = `cat $pid_file`;
	chomp $pid; # Newline entfernen

	unless ($pid =~ /^(\d+)$/) { # PID ist keine Nummer
	    print "\nFEHLER: PID-Datei \"$pid_file\" ist korrupt!\n";
	    $running = -1; # unbekannt
	}
	else {
	    `ps ax | grep -q -E "^ +$pid.*dlsd"`;
	    $running = 1 unless $?;
	}
    }

    # Informationen f�r die Ausgabe auswerten
    $proc = "UNBEKANNT";
    $proc = "laeuft" if $running == 1;
    $proc = "LAEUFT NICHT!" if $running == 0;
    $proc = "" if $state eq "paused";
    $desc = "UNBEKANNT" if $desc eq "";
    $state = "UNBEKANNT" if $state eq "";

    # Felder formatieren
    $job_id_6 = sprintf "%6d", $job_id;
    $desc_21 = sprintf "%21s", $desc;
    $state_9 = sprintf "%9s", $state;
    $proc_13 = sprintf "%13s", $proc;

    # Felder in Tabellenzeile ausgeben
    push @table, "| $job_id_6 | $desc_21 | $state_9 | $proc_13 |";
}

#----------------------------------------------------------------
