#!/usr/bin/perl
my $body = "CGI OK\n";
print "Content-Type: text/plain\n";
print "Content-Length: " . length($body) . "\n";
print "\n";
print $body;
