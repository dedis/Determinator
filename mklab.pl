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

		my $line = $_;
		my $enabled = ($#pass < 0 || $pass[0]);
		my $doprint = $enabled;

		if (m|^[#]if\s+LAB\s*[>][=]\s*(\d+)|) {

			# Collapse "#if LAB >= N" blocks
			$doprint = 0;
			if ($enabled) {
				unshift (@pass, $labno >= $1);
			} else {
				unshift (@pass, $pass[0]);
			}

		} elsif (m|^[#]if\s+SOL\s*[>][=]\s*(\d+)|) {

			# Collapse "#if SOL >= N" blocks
			$doprint = 0;
			if ($enabled) {
				unshift (@pass, $solno >= $1);
			} else {
				unshift (@pass, $pass[0]);
			}

		} elsif (m|^[#]if|) {

			# Do not collapse other types of ifdefs,
			# but keep track of them to preserve proper nesting.
			if ($enabled) {
				unshift (@pass, 2);
			} else {
				unshift (@pass, $pass[0]);
			}

		} elsif (m|^[#]else|) {

			if ($#pass >= 1 && $pass[0] == 2) {
				# Not an ifdef we're collapsing - do nothing.
			} elsif ($enabled) {
				# Output was enabled, so disable it
				$pass[0] = 0;
				$doprint = 0;
			} elsif ($#pass >= 1 && !$pass[1]) {
				# Output was disabled by enclosing ifdef,
				# so keep it disabled.
				$pass[0] = 0;
				$doprint = 0;
			} else {
				# Output was disabled by current ifdef,
				# so enable it now
				$pass[0] = 1;
				$doprint = 0;
			}

		} elsif (m|^[#]endif|) {
			if ($#pass >= 0) {
				if ($pass[0] < 2) {
					$doprint = 0;
				}
				shift (@pass);
			} else {
				print "Warning: unmatched #endif in $filename\n";
			}
		}

		# Copy the line to the output file if appropriate
		if ($doprint) {
			print OUTFILE $line;
			$lines++;
		}
	}

	if ($#pass >= 0) {
		print "Warning: unmatched #if/#ifdef in $filename\n";
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

