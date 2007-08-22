
use strict;

sub version2arr {
    my ($v) = @_;
    return split /\./, $v;
}

sub version_cmp {
    my ($a, $b) = @_;
    return 0 if ($#$a < 0 && $#$b < 0);
    return 1 if ($#$a >= 0 && $#$b < 0);
    return -1 if ($#$a < 0 && $#$b >= 0);
    return 1 if ($a->[0] > $b->[0]);
    return -1 if ($a->[0] < $b->[0]);
    shift (@$a); shift (@$b); 
    return version_cmp ($a, $b);
}

if ($#ARGV != 3) {
    warn "usage: vcmp.pl <a> <b> <if-geq> <if-lt>\n";
    exit (1);
}

my ($a,$b) = map { [ version2arr($_) ] } ($ARGV[0], $ARGV[1]);

my $rc = version_cmp ($a, $b);
if ($rc >= 0) { print $ARGV[2]; }
else {          print $ARGV[3]; }
