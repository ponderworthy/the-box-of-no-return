#!/bin/sh

MYDIR="`dirname $0`"
INSTR_DB="$MYDIR/../instruments.db"
SQLITE="`which sqlite3`"

echo "Creating the instrument's DB..."

if [ ! -x "$SQLITE" ] ; then
    echo "Can't find sqlite3" 1>&2
    exit -1
fi

if [ -e "$INSTR_DB" ] ; then
    echo "DB file exists ($INSTR_DB); Skipping..."
    echo "To recreate the instrument DB remove the old DB file first."
    exit
fi

$SQLITE -line $INSTR_DB < "$MYDIR/create_instr_db.sql"

echo "Done"
