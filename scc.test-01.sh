#!/bin/ksh
#
# @(#)$Id: scc.test-01.sh,v 1.3 2014/06/16 11:41:34 jleffler Exp $
#
# Test driver for SCC

T_SCC=./scc             # Version of SCC under test
R_SCC=$HOME/bin/scc     # Reference version of SCC

[ -x "$T_SCC" ] || ${MAKE:-make} "$T_SCC" || exit 1
[ -x "$R_SCC" ] || { echo "Reference version $R_SCC missing"; exit 1; }

usage()
{
    echo "Usage: $(basename $0 .sh) [-q]" >&2
    exit 1
}

while getopts q opt
do
    case "$opt" in
    (q) qflag=yes;;
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
    # Use printf because some echo command interpet -n
    file=$(printf "%s" $file)
    [ -f $file ] || ${CO:-co} $file || exit 1
    "$R_SCC" $r_opts $file > $tmp.1 2>$tmp.3
    "$T_SCC" $t_opts $file > $tmp.2 2>$tmp.4
    if [ -s $tmp.1 ] && [ -s $tmp.2 ] && cmp -s $tmp.1 $tmp.2 && cmp -s $tmp.3 $tmp.4
    then
        echo "== PASS == ($r_opts vs $t_opts on $file)"
        : $((pass++))
    else
        echo "!! FAIL !! ($r_opts vs $t_opts on $file)"
        [ "$qflag" = "yes" ] || diff $tmp.1 $tmp.2
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
