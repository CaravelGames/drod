#!/usr/bin/perl
use strict;

my @tagstack;

my $stream=\*STDIN;
my $fname=shift;

if ($fname) {
	open (FILE, "<$fname") or die "Couldn't open $fname\: $!";
	$stream=\*FILE;
}

my $data;

while (<$stream>) {
	chomp;
	$data.=$_;
	while ($data=~/^[^\<\>]*\<([^\<\>]*)\>(.*)$/) {
		my $tag=$1;
		$data=$2;
		if ($tag=~/^\s*\/(.*)$/) {
			my $toptag=pop @tagstack;
			$tag=$1;
			die "Opening $toptag doesn't match closing $tag" unless $tag eq $toptag;
		} elsif ($tag=~/\/$/) {
		} else {
			$tag=~/^(\S+)(?:\s+.*)?$/ or die "Tag error";
			$tag=$1;
			push @tagstack, $tag;
		}
	}
}
die "Didn't understand \"$data\"" if $data=~/\<|\>/;


if (scalar @tagstack) {
	my $msg="Mismatched tags: ";
	foreach my $tag (@tagstack) {
		$msg.=" $tag";
	}
	die $msg;
}

print "Success";
