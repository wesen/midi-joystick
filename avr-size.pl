#!/usr/bin/perl -w

open BLA, "avr-objdump -h ".join(" ", @ARGV)."|" || die "could not run avr-size";

# print join(" ", @ARGV)."\n";
my %sizes;
while (<BLA>) {
  if (/\s*[0-9]+\s+\.text\.([^\s]+)\s+([^\s]+)/) {
    my $name = $1;
    my $size = hex($2);
    $sizes{$name} = $size;
  }
}

my $sum = 0;

my @sorted = sort { $sizes{$a} <=> $sizes{$b} } keys %sizes;
foreach my $name (reverse @sorted) {
  format =
@>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> @#########
$name, $sizes{$name}
.
  write;
  $sum += $sizes{$name};
#  print "$name\t\t".$sizes{$name}."\n";
}

print "\nSum: $sum bytes\n";
