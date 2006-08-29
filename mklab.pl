#!/usr/bin/perl
#
# Generate lab/solution hand-out trees from master ossrc tree.
# Usage: mklab.pl lab# sol# outdir files
# - lab# is the lab number: 1, 2, 3, etc.
# - sol# is the solution set number: 1, 2, 3, etc.
# - outdir is the name of the directory into which to export the tree.
# - files is the complete set of source files to (potentially) copy.
#
# Blocks of text demarked with ///LABn and ///END are included
# only if the lab number is greater than or equal to 'n'.
# Blocks of text demarked with ///SOLn and ///END are included
# only if the solution set number is greater than or equal to 'n'.
#

sub dofile {
	my $filename = shift;
	my $ccode = 0;
	if ($filename =~ /.*\.[ch]$/) { $ccode = 1; }
	my $tmpfilename = "$filename.tmp";

	open(INFILE, "<$filename") or die "Can't open $filename";
	open(OUTFILE, ">$tmpfilename") or die "Can't open $tmpfilename";

	my $outlines = 0;
	my $inlines = 0;
	my $expectedinline = 1;
	my $lastgood = "NO LAST GOOD";
	my %stack;
	my $depth = 0;
	$stack{$depth}->{'emit'} = 1;

	while(<INFILE>) {
		my $line = $_;
		chomp;
		$inlines++;
		$emit = $stack{$depth}->{'emit'};
		if (m:^(ifdef|ifndef|[#]elif|[#]if|/\*#if|/\*#elif)\s+(LAB|SOL)\s*([=<>!][=]|[<>]|)\s*(\d+):) {
			# Parse a new condition
			my $val = ($2 eq 'LAB' ? $labno : $solno);
			my $cond;
			if ($3 eq '' || $3 eq '>=') {
				$cond = $val >= $4;
			} elsif ($3 eq '==') {
				$cond = $val == $4;
			} elsif ($3 eq '<') {
				$cond = $val < $4;
			} elsif ($3 eq '>') {
				$cond = $val > $4;
			} elsif ($3 eq '<=') {
				$cond = $val <= $4;
			} elsif ($3 eq '!=') {
				$cond = $val != $4;
			} else {
				die;
			}
			if ($1 eq "#if" or $1 eq "ifdef" or $1 eq "ifndef" or $1 eq "/*#if") {
				$stack{++$depth} = { 'anytrue' => 0, 'isours' => 1 };
			}
			$stack{$depth}->{'emit'} = ($stack{$depth-1}->{'emit'} && $cond);
			if ($1 eq "#elif" or $1 eq "elif" or $1 eq "/*elif") {
				$stack{$depth}->{'emit'} &= !$stack{$depth}->{'anytrue'};
			}
			$stack{$depth}->{'anytrue'} |= $cond;
			$emit = 0;
			$lastgood = $line;
                } elsif (m:^[#]?(ifdef|ifndef)\s+ENV_(\w+):) {
			   $cond = defined ($ENV{$2}) && $ENV{$2};
			   $cond = !$cond if $1 eq "ifndef";
			   $stack{++$depth} = { 'anytrue' => $cond,
						'isours' => 1 };
			   $stack{$depth}->{'emit'} = ($stack{$depth-1}->{'emit'} && $cond);
			   $emit = 0;
			   $lastgood = $line;
		} elsif (m:^([#]if|ifdef|ifndef|ifeq|ifneq):) {
			# Other conditions we just pass through
			++$depth;
			$stack{$depth} = { 'isours' => 0, 'emit' => $stack{$depth-1}->{'emit'} };
			$lastgood = $line;
		} elsif (m:^(/\*[#]|[#]|)else:) {
			if ($stack{$depth}->{'isours'}) {
				$emit = 0;
				$stack{$depth}->{'emit'} = ($stack{$depth-1}->{'emit'} && !$stack{$depth}->{'anytrue'});
			}
			$lastgood = $line;
		} elsif (m:^(/\*[#]|[#]|)endif:) {
			if ($stack{$depth}->{'isours'}) {
				$emit = 0;
			}
			--$depth;
			if ($depth < 0) {
				die("unmatched endif on line $filename:$inlines \"$line\"");
			}

		}

		if ($emit) {
			if ($solno > 0 && $ccode &&
			    $expectedinline != $inlines) {
				print OUTFILE "#line $inlines \"../$filename\"\n";
				$expectedinline = $inlines;
			}
			print OUTFILE "$_\n";
			$outlines++;
			$expectedinline++;
		}
	}

	if ($depth != 0) {
		print STDERR "warning: unmatched #if/#ifdef/#elif/#else/#endif in $filename\n";
		print STDERR "last known good line: $filename:$inlines $lastgood\n";
	}

	close INFILE;
	close OUTFILE;

	# After doing all that work, nuke empty files
	if ($outlines) {
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

if (@ARGV < 3) {
	usage();
}

$labno = shift(@ARGV);
$solno = shift(@ARGV);
$outdir = shift(@ARGV);

if ($labno < 1 || $solno !~ /^0|[1-9][0-9]*$/) {
	usage();
}

# Create a new output directory.
if (!(-d $outdir)) {
	mkdir($outdir) or die "mkdir $outdir: $!";
}

# Populate the output directory
foreach $i (@ARGV) {
	dofile($i);
}
