#!/bin/ksh
#
# @(#)$Id: scc.test-01.sh,v 1.8 2016/06/10 04:37:06 jleffler Exp $
#
# Test driver for SCC - comparing with reference version (SCC 5.05)

: ${CO:=co}
: ${DIFF:=diff}

T_SCC=./scc             # Version of SCC under test
R_SCC=./scc-5.05        # Reference version of SCC

[ -x "$T_SCC" ] || ${MAKE:-make} "$T_SCC" || exit 1
[ -x "$R_SCC" ] || { echo "Reference version $R_SCC missing"; exit 1; }

R_BASE=$(basename "$R_SCC")
T_BASE=$(basename "$T_SCC")

usage()
{
    echo "Usage: $(basename $0 .sh) [-v]" >&2
    exit 1
}

vflag=no
while getopts v opt
do
    case "$opt" in
    (v) vflag=yes;;
    (*) usage;;
    esac
done
shift $((OPTIND - 1))
[ "$#" = 0 ] || usage

tmp=${TMPDIR:-/tmp}/scc-test
trap "rm -f $tmp.?; exit 1" 0 1 2 3 13 15

# Test file names may not contain spaces or shell metacharacters
sed -e 's/#.*//' -e '/^ *$/d' << EOF |

        |             | scc-test.example1.c
-C      | -S c89      | scc-test.example1.c
-C -w   | -S c89 -w   | scc-test.example1.c
-C -s X | -S c89 -s X | scc-test.example1.c
-C -q Y | -S c89 -q Y | scc-test.example1.c
-C -n   | -S c89 -n   | scc-test.example1.c
-n      | -S c99 -n   | scc-test.example1.c
-n      | -S c11 -n   | scc-test.example1.c
-n      | -S c++98 -n | scc-test.example1.c
-n      | -S c++03 -n | scc-test.example1.c
-n      | -S c++11 -n | scc-test.example1.c
-n      | -S c++14 -n | scc-test.example1.c
-n      | -S c -n     | scc-test.example1.c
-n      | -n          | scc-test.example1.c
-n      | -S c++ -n   | scc-test.example1.c
-c      | -S c++ -c   | scc-test.example1.c
-c -C   | -S c89 -c   | scc-test.example1.c

EOF

{
fail=0
pass=0
# Don't quote file name or options - spaces need trimming
while IFS="|" read r_opts t_opts file
do
    # Use printf because some echo commands interpet -n
    file=$(printf "%s" $file)
    [ -f $file ] || ${CO} $file || exit 1
    "$R_SCC" $r_opts $file > $tmp.1 2>$tmp.3
    "$T_SCC" $t_opts $file > $tmp.2 2>$tmp.4
    # Old SCC misreported the line number when there's an unescaped
    # newline in a character or string constant.
    # Old SCC ignored C++ comments in C90 code, but new SCC reports
    # 'Double slash comment used when not supported' or similar,
    # Old SCC is run as scc-5.05; normalize error messages from new SCC
    # to use that name too (to simplify comparisons).
    sed -e 's/:63:/:64:/' \
        -e '/Double slash comment used/d' \
        -e '/Universal character names feature/d' \
        -e "s/^$T_BASE:/$R_BASE:/" \
        $tmp.4 > $tmp.5
    if [ -s $tmp.1 ] && [ -s $tmp.2 ] && cmp -s $tmp.1 $tmp.2 &&
        cmp -s $tmp.3 $tmp.5
    then
        echo "== PASS == ($r_opts vs $t_opts on $file)"
        : $((pass++))
    else
        echo "!! FAIL !! ($r_opts vs $t_opts on $file)"
        if [ "$vflag" = "yes" ]
        then
            ${DIFF} $tmp.1 $tmp.2
            ${DIFF} $tmp.3 $tmp.4
        fi
        : $((fail++))
    fi
    rm -f $tmp.?
done
if [ $fail = 0 ]
then echo "== PASS == ($pass tests OK)"
else echo "!! FAIL !! ($pass tests OK, $fail tests failed)"
fi
}

rm -f $tmp.?
trap 0
