#!/bin/ksh
#
# @(#)$Id: scc.test-04.sh,v 1.3 2016/06/01 06:33:06 jleffler Exp $
#
# Test driver for SCC: -S C++14 and binary numbers, numeric punctuation

T_SCC=./scc             # Version of SCC under test

[ -x "$T_SCC" ] || ${MAKE:-make} "$T_SCC" || exit 1

usage()
{
    echo "Usage: $(basename $0 .sh) [-gq]" >&2
    exit 1
}

gflag=no
qflag=no
while getopts gq opt
do
    case "$opt" in
    (g) gflag=yes;;
    (q) qflag=yes;;
    (*) usage;;
    esac
done
shift $((OPTIND - 1))
[ "$#" = 0 ] || usage

tmp=${TMPDIR:-/tmp}/scc-test
trap "rm -f $tmp.?; exit 1" 0 1 2 3 13 15

OUTPUT_DIR=Output
TEST_BINARY=scc-test.binary
TEST_NUMPUNCT=scc-test.numpunct
BOGUS_BINARY=scc-bogus.binary
TEST_HEXFLOAT=scc-test.hexfloat

{
fail=0
pass=0
# Don't quote file name or options - spaces need trimming
for base in $TEST_BINARY $TEST_NUMPUNCT $BOGUS_BINARY
do
    for standard in c c++11 c++14 c++17
    do
        test=0
        "$T_SCC" -n -S $standard "${base}.cpp" > "$tmp.1" 2> "$tmp.2"
        EXPOUT="$OUTPUT_DIR/$base-$standard.1"
        EXPERR="$OUTPUT_DIR/$base-$standard.2"
        if [ "$gflag" = yes ]
        then cp "$tmp.1" "$EXPOUT"
        elif cmp -s "$tmp.1" "$EXPOUT"
        then : OK
        else
            echo "Differences: $base.cpp - standard output"
            diff "$tmp.1" "$EXPOUT"
            test=1
        fi
        if [ "$gflag" = yes ]
        then cp "$tmp.2" "$EXPERR"
        elif cmp -s "$tmp.2" "$EXPERR"
        then : OK
        else
            echo "Differences: $base.cpp - standard error"
            diff "$tmp.2" "$EXPERR"
            test=1
        fi
        if [ $test = 0 ]
        then
            echo "== PASS == ($base.cpp)"
            : $((pass++))
        else
            echo "!! FAIL !! ($base.cpp)"
            : $((fail++))
        fi
        rm -f "$tmp".?
    done
done
if [ $fail = 0 ]
then echo "== PASS == ($pass tests OK)"
else echo "!! FAIL !! ($pass tests OK, $fail tests failed)"
fi
}

rm -f $tmp.?
trap 0
