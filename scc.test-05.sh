#!/bin/ksh
#
# @(#)$Id: scc.test-05.sh,v 1.4 2016/06/13 05:45:22 jleffler Exp $
#
# Test driver for SCC: Handling of Unicode characters and strings

T_SCC=./scc             # Version of SCC under test

[ -x "$T_SCC" ] || ${MAKE:-make} "$T_SCC" || exit 1

usage()
{
    echo "Usage: $(basename $0 .sh) [-gq]" >&2
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

tmp=${TMPDIR:-/tmp}/scc-test
trap "rm -f $tmp.?; exit 1" 0 1 2 3 13 15

OUTPUT_DIR=Output
EXAMPLE_C=scc-test.example3
UCNS_OK=scc-test.ucns
UCNS_BOGUS=scc-bogus.ucns

{
fail=0
pass=0
# Don't quote file name or options - spaces need trimming
for base in $EXAMPLE_C $UCNS_OK $UCNS_BOGUS
do
    for standard in c90 c99 c11 c++98 c++11 c++14 c++17
    do
        test=0
        "$T_SCC" -n -S "$standard" "${base}.c" > "$tmp.1" 2> "$tmp.2"
        EXPOUT="$OUTPUT_DIR/$base-$standard.1"
        EXPERR="$OUTPUT_DIR/$base-$standard.2"
        if [ "$gflag" = yes ]
        then cp "$tmp.1" "$EXPOUT"
        elif cmp -s "$tmp.1" "$EXPOUT"
        then : OK
        else
            echo "Differences: $base-$standard.c - standard output"
            diff "$tmp.1" "$EXPOUT"
            test=1
        fi
        if [ "$gflag" = yes ]
        then cp "$tmp.2" "$EXPERR"
        elif cmp -s "$tmp.2" "$EXPERR"
        then : OK
        else
            echo "Differences: $base-$standard.c - standard error"
            diff "$tmp.2" "$EXPERR"
            test=1
        fi
        if [ "$gflag" = yes ]
        then
            echo "== GENERATED == ($standard: $base.c)"
            echo "                ($EXPOUT $EXPERR)"
            : $((pass++))
        elif [ $test = 0 ]
        then
            echo "== PASS == ($standard: $base.c)"
            : $((pass++))
        else
            echo "!! FAIL !! ($standard: $base.c)"
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
