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
	my %stack;
	my $depth = 0;
	$stack{$depth}->{'emit'} = 1;

	while(<INFILE>) {
		my $line = $_;
		chomp;
		$emit = $stack{$depth}->{'emit'};
		if (m:^[#](elif|if)\s+(LAB|SOL)\s*[>][=]\s*(\d+):) {
			# Parse a new condition
			if ($2 eq "LAB") {
				$cond = ($labno >= $3);
			} else {
				$cond = ($solno >= $3);
			}
			if ($1 eq "if") {
				$stack{++$depth} = { 'anytrue' => 0, 'isours' => 1 };
			}
			$stack{$depth}->{'emit'} = ($stack{$depth-1}->{'emit'} && $cond);
			$stack{$depth}->{'anytrue'} |= $cond;
			$emit = 0;
		} elsif (m:^[#]if:) {
			# Other conditions we just pass through
			++$depth;
			$stack{$depth} = { 'isours' => 1, 'emit' => $stack{$depth-1}->{'emit'} };
		} elsif (m:^[#]else:) {
			if ($stack{$depth}->{'isours'}) {
				$emit = 0;
				$stack{$depth}->{'emit'} = ($stack{$depth-1}->{'emit'} && !$stack{$depth}->{'anytrue'});
			}
		} elsif (m:^[#]endif:) {
			if ($stack{$depth}->{'isours'}) {
				$emit = 0;
			}
			--$depth;
		}

		if ($emit) {
			print OUTFILE "$_\n";
			$lines++;
		}
	}

	if ($depth != 0) {
		print STDERR "warning: unmatched #if/#ifdef/#elif/#else/#endif in $filename\n";
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

sub usage { 
	print STDERR "usage: mklab <labno> <solno> <outdir> <srcfiles...>\n";
	exit(1);
}

if(@ARGV < 3) {
	usage();
}

$labno = shift(@ARGV);
$solno = shift(@ARGV);
$outdir = shift(@ARGV);

if ($labno < 1 || $solno !~ /^0|[1-9][0-9]*$/) {
	usage();
}

# Create a new output directory.
mkdir($outdir, 0777) or die "mkdir $outdir: $!";

# Populate the output directory
foreach $i (@ARGV) {
	dofile($i);
}
