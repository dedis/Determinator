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

	my $level = $labno*2+$sols;
	my $lines = 0;
	my @pass;
	unshift (@pass, 200);
	while (<$INFILE>) {
		if (m|///LAB(\d+)|) {
			if ($level >= $pass[0]) {
				unshift (@pass, $1*2);
			} else {
				unshift (@pass, $pass[0]);
			}
		} elsif (m|///SOL(\d+)|) {
			if ($level >= $pass[0]) {
				unshift (@pass, $1*2+1);
			} else {
				unshift (@pass, $pass[0]);
			}
		} elsif (m|///END|) {
			shift (@pass);
		} elsif ($level >= $pass[0]) {
			print OUTFILE $_;
			$lines++;
		}
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
} else {
	$outdir = "sol$labno";
}

# Blow away the old output directory and create a new one
mkdir($outdir, 0777) or die "Can't create subdirectory $outdir";

# Populate the output directory
foreach $i (1 .. $#ARGV) {
	dofile($ARGV[$i]);
}

