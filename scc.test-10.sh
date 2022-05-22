#!/bin/ksh
#
# @(#)$Id: scc.test-10.sh,v 8.1 2022/04/07 00:40:18 jleffler Exp $
#
# Test driver for SCC: Report bogus end comments with bsnl

T_SCC=./scc             # Version of SCC under test
RCSKWCMP="${RCSKWCMP:-./rcskwcmp}"
RCSKWREDUCE="${RCSKWREDUCE:-./rcskwreduce}"
export RCSKWCMP RCSKWREDUCE

[ -x "$T_SCC" ] || ${MAKE:-make} "$T_SCC" || exit 1

arg0=$(basename "$0" .sh)

usage()
{
    echo "Usage: $arg0 [-gq]" >&2
    exit 1
}

# -g  Generate result files
# -q  Quiet mode

qflag=no
gflag=no
while getopts gq opt
do
    case "$opt" in
    (q) qflag=yes;;
    (g) gflag=yes;;
    (*) usage;;
    esac
done
shift $((OPTIND - 1))
[ "$#" = 0 ] || usage

tmp="${TMPDIR:-/tmp}/scc-test.$$"
trap "rm -f $tmp.?; exit 1" 0 1 2 3 13 15

OUTPUT_DIR=Output
SOURCE=scc-bogus.endcomment.c
base="$arg0.endcomment"

{
suffix=".c"
fail=0
pass=0
# Don't quote file name or options - spaces need trimming
for options in "-e" "-n" "-c" "-t"
do
    test=0
    "$T_SCC" "$options" "${SOURCE}" > "$tmp.1" 2> "$tmp.2"
    EXPOUT="$OUTPUT_DIR/$base$options.1"
    EXPERR="$OUTPUT_DIR/$base$options.2"
    if [ "$gflag" = yes ]
    then cp "$tmp.1" "$EXPOUT"
    elif $RCSKWCMP "$tmp.1" "$EXPOUT"
    then : OK
    else
        echo "Differences: $SOURCE - standard output (wanted vs actual)"
        diff "$EXPOUT" "$tmp.1"
        test=1
    fi
    if [ "$gflag" = yes ]
    then cp "$tmp.2" "$EXPERR"
    elif $RCSKWCMP "$tmp.2" "$EXPERR"
    then : OK
    else
        echo "Differences: $SOURCE - standard error (wanted vs actual)"
        diff "$EXPERR" "$tmp.2"
        test=1
    fi
    if [ "$gflag" = yes ]
    then
        echo "== GENERATED == ($options: $SOURCE)"
        echo "                ($EXPOUT $EXPERR)"
        : $((pass++))
    elif [ $test = 0 ]
    then
        echo "== PASS == ($options: $SOURCE)"
        : $((pass++))
    else
        echo "!! FAIL !! ($options: $SOURCE)"
        : $((fail++))
    fi
    rm -f "$tmp".?
done
if [ $fail = 0 ]
then echo "== PASS == ($pass tests OK)"
else echo "!! FAIL !! ($pass tests OK, $fail tests failed)"
fi
}

rm -f $tmp.?
trap 0
