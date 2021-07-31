#!/usr/bin/perl -w

# Updates grammer in lscp.xml
#
# Updates the "Command Syntax" section of Documentation/lscp.xml with current
# LSCP grammar definition from network/lscp.y which is the yacc input file
# used to automatically generate LinuxSampler's LSCP parser C++ source file
# (network/lscpparser.cpp).
#
# Usage: update_grammar.pl [-v]
#
#    -v  verbose output

my $YACC_FILE          = "../src/network/lscp.y";
my $XML2RFC_FILE       = "../Documentation/lscp.xml";
my $ANCHOR_TOKEN_BEGIN = "GRAMMAR_BNF_BEGIN";
my $ANCHOR_TOKEN_END   = "GRAMMAR_BNF_END";

my $verbose = 0;
if (defined($ARGV[0]) and $ARGV[0] eq "-v") {
    $verbose = 1;
}

open(IN, $YACC_FILE) || die "Can't open yacc input file for creating xml2rfc file.";
open(OUT, $XML2RFC_FILE) || die "Can't read xml2rfc file.";


my @yacc_in         = <IN>;
my $in_marker_begin = -1;
my $in_marker_end   = -1;
my $i               = 0;

# scan the yacc input file for the marker lines
if ($verbose) { print "Scanning file \"${YACC_FILE}\" ...\n"; }
foreach $line (@yacc_in) {
    if    ($line =~ /$ANCHOR_TOKEN_BEGIN/o) { $in_marker_begin = $i; }
    elsif ($line =~ /$ANCHOR_TOKEN_END/o)   { $in_marker_end   = $i; }
    $i++;
}
if ($in_marker_begin < 0) {
    die "Did not find ${ANCHOR_TOKEN_BEGIN} token in ${YACC_FILE}, exiting!";
}
if ($in_marker_end < 0) {
    die "Did not find ${ANCHOR_TOKEN_END} token in ${YACC_FILE}, exiting!";
}
if ($in_marker_end <= $in_marker_begin) {
    die "Marker line ${ANCHOR_TOKEN_END} appears before marker line ${ANCHOR_TOKEN_BEGIN}, exiting!";
}
if ($verbose) { print "Ok, found ${ANCHOR_TOKEN_BEGIN} in line ${in_marker_begin} and ${ANCHOR_TOKEN_END} in line ${in_marker_end}.\n"; }
# delete everything except the grammar lines
splice(@yacc_in, $in_marker_end, $#yacc_in - $in_marker_end);
splice(@yacc_in, 0, $in_marker_begin + 1);

# remove C++ code
$scalar_yacc_in = join("", @yacc_in);
$scalar_yacc_in =~ s/'\{'|'\}'//mg;
$scalar_yacc_in =~ s/\{(\d|[a-z]|[A-Z]|\#|;|:|<|>|\(|\)|\$|\[|\]|=|\+|-|\"|'|_|\\|\/|\.|,|\s|\n|\r)*\}//mgix;
# remove surving '}' character ;-)
#$scalar_yacc_in =~ s/\}/\n/g;

# convert scalar, long string into a line array
@yacc_in = split(/\n/, $scalar_yacc_in);

# do the XML transformation
$i = 0;
foreach $line (@yacc_in) {
    $_ = $line . "\n";
    # remove C++ code
    s/\{\p{IsASCII}*\}//g;
    s/\/\/\p{IsASCII}*$/\r\n/g;
    s/\{//g;
    s/\}//g;
    # place XML tags
    s!^(\w+)\s*:\s*(\p{IsASCII}*)!<t>$1 =\r\n\t<list>\r\n\t\t<t>$2!;
    s!^\s+\|\s*(\p{IsASCII}*)!\t\t</t>\r\n\t\t<t>/ $1!;
    s!^\s*;(\p{IsASCII}*)!\t\t</t>\r\n\t</list>\r\n</t>!;
    #s/:/<list>/g;
    # remove space(s) at the end of each line
    s/\s+$/\r\n/g;
    $yacc_in[$i] = $_;
    $i++;
}

my @xml_in           = <OUT>;
my $out_marker_begin = -1;
my $out_marker_end   = -1;
   $i                = 0;

# scan the xml2rfc file for the marker lines
if ($verbose) { print "Scanning file \"${XML2RFC_FILE}\" ...\n"; }
foreach $line (@xml_in) {
    if    ($line =~ /$ANCHOR_TOKEN_BEGIN/o) { $out_marker_begin = $i; }
    elsif ($line =~ /$ANCHOR_TOKEN_END/o)   { $out_marker_end   = $i; }
    $i++;
}
if ($out_marker_begin < 0) {
    die "Did not find ${ANCHOR_TOKEN_BEGIN} token in ${XML2RFC_FILE}, exiting!";
}
if ($out_marker_end < 0) {
    die "Did not find ${ANCHOR_TOKEN_END} token in ${XML2RFC_FILE}, exiting!";
}
if ($out_marker_end <= $out_marker_begin) {
    die "Marker line ${ANCHOR_TOKEN_END} appears before marker line ${ANCHOR_TOKEN_BEGIN}, exiting!";
}
if ($verbose) { print "Ok, found ${ANCHOR_TOKEN_BEGIN} in line ${out_marker_begin} and ${ANCHOR_TOKEN_END} in line ${out_marker_end}.\n"; }
close(OUT); # we reopen in write (clobber) mode

# finally replace the area in the output xml file with the new grammar
splice(@xml_in, $out_marker_begin + 1, $out_marker_end - $out_marker_begin - 1, @yacc_in);
open(OUT, "+>", $XML2RFC_FILE) || die "Can't open xml2rfc file for output";
print OUT @xml_in;
close(OUT);
close(IN);

if ($verbose) { print "Done.\n"; }

exit(0);
