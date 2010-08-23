#!/usr/bin/perl
#
# This script pads a boot block to the required 512 byte sector size
# and attaches the magic "signature" (0x55aa) at the end.
#
# Copyright (C) 1997 Massachusetts Institute of Technology 
# See section "MIT License" in the file LICENSES for licensing terms.
#
# Derived from the MIT Exokernel and JOS.
#

open(BB, $ARGV[0]) || die "open $ARGV[0]: $!";

binmode BB;
my $buf;
read(BB, $buf, 1000);
$n = length($buf);

if($n > 510){
	print STDERR "boot block too large: $n bytes (max 510)\n";
	exit 1;
}

print STDERR "boot block is $n bytes (max 510)\n";

$buf .= "\0" x (510-$n);
$buf .= "\x55\xAA";

open(BB, ">$ARGV[0]") || die "open >$ARGV[0]: $!";
binmode BB;
print BB $buf;
close BB;
