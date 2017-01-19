#!/bin/bash
#
# @(#)$Id: scc.test-06.sh,v 1.2 2016/05/22 20:52:24 jleffler Exp $
#
# Test driver for SCC: using -c, -n in combination
#
# NB: This is a Bash script because it uses command substitution

T_SCC=./scc             # Version of SCC under test
RCSKW="${RCSKWREDUCE:-rcskwreduce}"

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
EXAMPLE_C=scc-test.example2

{
fail=0
pass=0
# Don't quote file name or options - spaces need trimming
for standard in c90 c99 c++98
do
    base=$EXAMPLE_C
    for args in "" "-c" "-n" "-c -n"
    do
        test=0
        "$T_SCC" $args -S "$standard" "${base}.c" > "$tmp.1" 2> "$tmp.2"
        EXPECT="$base-$standard${args/ /}"
        EXPOUT="$OUTPUT_DIR/$EXPECT.1"
        EXPERR="$OUTPUT_DIR/$EXPECT.2"
        if [ "$gflag" = yes ]
        then cp "$tmp.1" "$EXPOUT"
        elif cmp -s <($RCSKW "$tmp.1") <($RCSKW "$EXPOUT")
        then : OK
        else
            echo "Differences: $EXPECT - standard output"
            diff "$tmp.1" "$EXPOUT"
            test=1
        fi
        if [ "$gflag" = yes ]
        then cp "$tmp.2" "$EXPERR"
        elif cmp -s <($RCSKW "$tmp.2") <($RCSKW "$EXPERR")
        then : OK
        else
            echo "Differences: $EXPECT - standard error"
            diff "$tmp.2" "$EXPERR"
            test=1
        fi
        if [ "$gflag" = no ]
        then
            if [ $test = 0 ]
            then
                echo "== PASS == ($standard: ${args:+$args }$base.c)"
                : $((pass++))
            else
                echo "!! FAIL !! ($standard: ${args:+$args }$base.c)"
                : $((fail++))
            fi
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
