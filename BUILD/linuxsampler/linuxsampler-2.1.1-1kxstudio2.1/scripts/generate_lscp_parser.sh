#!/bin/sh
#
# Generates the LSCP parser's C++ source files (src/network/lscpparser.cpp and
# src/network/lscpsymbols.h) according to the LSCP (BNF) grammar definition
# given by src/network/lscp.y

SCRIPTS_DIR=`dirname $0`
NETWORK_SRC_DIR="$SCRIPTS_DIR/../src/network"

echo -n "Searching for a parser generator..."
YACC_CMD=NONE
if which "bison" > /dev/null; then
    YACC_CMD="`which bison` -y"
elif which "yacc" > /dev/null; then
    YACC_CMD=`which yacc`
else
    echo "Error: You need yacc (or bison) to generate the LSCP parser !"
    exit -1
fi
echo "OK ($YACC_CMD)"

echo "Generating LSCP parser..."
(
    cd $NETWORK_SRC_DIR
    $YACC_CMD -d lscp.y
    $YACC_CMD lscp.y
    mv -f y.tab.h lscpsymbols.h 2>/dev/null
    mv -f y.tab.c lscpparser.cpp 2>/dev/null
)
echo "Done"

echo -n "Updating Documentation/lscp.xml..."
(cd $SCRIPTS_DIR && ./update_lscp_grammar.pl)
echo "Done"

echo -n "Generating src/network/lscp_shell_reference.cpp..."
(cd $SCRIPTS_DIR && ./generate_lscp_shell_reference.pl)
echo "Done"
