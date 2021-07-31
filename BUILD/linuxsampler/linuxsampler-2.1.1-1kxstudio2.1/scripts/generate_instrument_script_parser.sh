#!/bin/sh
#
# Generates the instrument script parser's C++ source files
# (parser.h, parser.cpp and scanner.cpp in and src/scriptvm/) according to the
# instrument script (BNF) grammar definition given by src/scriptvm/parser.y and
# the terminal symbol scanner/lexer defintion given by src/scriptvm/scanner.l

SCRIPTS_DIR=`dirname $0`
PARSER_SRC_DIR="$SCRIPTS_DIR/../src/scriptvm"

echo -n "Searching for lexer and parser generator..."

LEX_CMD=NONE
if which "flex" > /dev/null; then
    LEX_CMD=`which flex`
elif which "lex" > /dev/null; then
    LEX_CMD=`which lex`
else
    echo "Error: You need lex (or flex) to generate the instrument script parser !"
    exit -1
fi

YACC_CMD=NONE
if which "bison" > /dev/null; then
    YACC_CMD="`which bison` -y"
elif which "yacc" > /dev/null; then
    YACC_CMD=`which yacc`
else
    echo "Error: You need yacc (or bison) to generate the instrument script parser !"
    exit -1
fi

echo "OK ($LEX_CMD, $YACC_CMD)"

echo -n "Generating instrument script parser ... "
(
    cd $PARSER_SRC_DIR

    $LEX_CMD scanner.l
    #mv -f lex.yy.c scanner.cpp
    mv -f lex.InstrScript_.c scanner.cpp 2>/dev/null

    $YACC_CMD -d parser.y
    $YACC_CMD parser.y
    mv -f y.tab.h parser.h 2>/dev/null
    mv -f y.tab.c parser.cpp 2>/dev/null
)
echo "Done"
