#!/usr/bin/perl
#
# Generate lab/solution hand-out trees from master ossrc tree.
# Usage: mklab.pl lab# flag files
# - lab# is the lab number: 1, 2, 3, etc.
# - flag is 0 for the handout tree, 1 for the solution tree for that lab.
# - files is the complete set of source files to (potentially) copy.
#
# Blocks of text demarked with ///LABn and ///END are included
# only if the lab number is greater than or equal to 'n'.
# Blocks of text demarked with ///SOLn and ///END are included
# only if the lab number is greater than 'n',
# or if the lab number is equal to 'n' and the solutions flag is 1.
#

sub dofile {
	my $filename = shift;
	my $tmpfilename = "$filename.tmp";

	open(INFILE, "<$filename") or die "Can't open $filename";
	open(OUTFILE, ">$tmpfilename") or die "Can't open $tmpfilename";

	my $lines = 0;
	my @pass = ();
	while (<INFILE>) {
		if (m|[#]?///LAB(\d+)|) {
			if ($#pass < 0 || $pass[0]) {
				unshift (@pass, $labno >= $1);
			} else {
				unshift (@pass, $pass[0]);
			}
			#print "$_: @pass\n";
		} elsif (m|[#]?///SOL(\d+)|) {
			if ($#pass < 0 || $pass[0]) {
				unshift (@pass, $solno >= $1);
			} else {
				unshift (@pass, $pass[0]);
			}
			#print "$_: @pass\n";
		} elsif (m|[#]?///ELSE|) {
			if ($#pass < 0 || $pass[0] ||
					($#pass >= 1 && !$pass[1])) {
				$pass[0] = 0;
			} else {
				$pass[0] = 1;
			}
			#print "$_: @pass\n";
		} elsif (m|[#]?///END|) {
			if ($#pass >= 0) {
				shift (@pass);
			} else {
				print "Warning: unmatched ///END in $filename\n";
			}
			#print "$_: @pass\n";
		} elsif ($#pass < 0 || $pass[0]) {
			#print "$#pass--$pass[0]--$_";
			print OUTFILE $_;
			$lines++;
		}
	}

	if ($#pass >= 0) {
		print "Warning: unmatched ///LAB or ///SOL in $filename\n";
	}

	close INFILE;
	close OUTFILE;

	# After doing all that work, nuke empty files
	if ($lines) {
		my $outfilename = "$outdir/$filename";

		# Create the directory the output file lives in.
		$_ = $outfilename;
		s|[/][^/]*$||g;
		system("mkdir", "-p", $_);

		# Move the temporary file to the correct place.
		rename($tmpfilename, $outfilename);
	} else {
		unlink $tmpfilename;
	}
}

$labno = shift(@ARGV);
$sols = shift(@ARGV);

if ($labno < 1 || ($sols ne "0" && $sols ne "1")) {
	print "Usage: mklab <labno> <sols> <srcfiles...>\n";
	exit(1);
}

if ($sols == 0) {
	$outdir = "lab$labno";
	$solno = 0;
} else {
	$outdir = "sol$labno";
	$solno = $labno;
}

# Blow away the old output directory and create a new one
mkdir($outdir, 0777) or die "Can't create subdirectory $outdir";

# Populate the output directory
foreach $i (0 .. $#ARGV) {
	dofile($ARGV[$i]);
}

