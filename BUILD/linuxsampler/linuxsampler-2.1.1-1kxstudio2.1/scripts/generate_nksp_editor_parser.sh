#!/bin/sh
#
# Generates the NKSP syntax higlighting scanner's C++ source file
# (nksp.cpp in src/scriptvm/editor) according to the NKSP instrument script
# regular expressions given by src/scriptvm/editor/nksp.l. This is not used
# for the sampler's own instrument parser, but rather to provide syntax
# highlighting for NKSP documents for external text editor applications.

SCRIPTS_DIR=`dirname $0`
PARSER_SRC_DIR="$SCRIPTS_DIR/../src/scriptvm/editor"

echo -n "Searching for lexer..."

LEX_CMD=NONE
if which "flex" > /dev/null; then
    LEX_CMD=`which flex`
elif which "lex" > /dev/null; then
    LEX_CMD=`which lex`
else
    echo "Error: You need lex (or flex) to generate the NKSP syntax highlighting scanner !"
    exit -1
fi

echo "OK ($LEX_CMD)"

echo -n "Generating NKSP editor syntax highlighting scanner ... "
(
    cd $PARSER_SRC_DIR

    $LEX_CMD nksp.l
    mv -f lex.Nksp_.c nksp.cpp 2>/dev/null
)
echo "Done"
