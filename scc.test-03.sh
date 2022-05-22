#!/bin/ksh
#
# @(#)$Id: scc.test-03.sh,v 1.4 2018/11/11 22:49:27 jleffler Exp $
#
# Test driver for SCC: standard versions and feature sets

T_SCC=./scc             # Version of SCC under test

[ -x "$T_SCC" ] || ${MAKE:-make} "$T_SCC" || exit 1

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

sed -e 's/#.*//' -e '/^ *$/d' << EOF |

C     comments unicode.strings universal.character.names hexadecimal.float
C++   comments unicode.strings universal.character.names raw.strings
C89
C94
C99   comments universal.character.names hexadecimal.float
C11   comments universal.character.names hexadecimal.float unicode.strings
C18   comments universal.character.names hexadecimal.float unicode.strings
C++98 comments universal.character.names
C++03 comments universal.character.names
C++11 comments universal.character.names raw.strings unicode.strings
C++14 comments universal.character.names raw.strings unicode.strings binary numeric.punctuation
C++17 comments universal.character.names raw.strings unicode.strings binary numeric.punctuation hexadecimal.float

EOF

{
fail=0
pass=0
# Don't quote file name or options - spaces need trimming
while read std features
do
    "$T_SCC" -f -S $std /dev/null > $tmp.1 2>$tmp.2
    if [ -s $tmp.2 ]
    then
        echo "!! FAIL !! (Standard = $std - non-empty error output)"
        : $((fail++))
    elif [ -z "$features" ]
    then
        if grep "Feature:" $tmp.1 >/dev/null
        then
            echo "!! FAIL !! (Standard = $std - non-empty feature output)"
            [ "$qflag" = "yes" ] || cat $tmp.1
            : $((fail++))
        else
            echo "== PASS == (Standard = $std)"
            : $((pass++))
        fi
    else
        tfail=0
        for feature in $features
        do
            if grep -i -e "Feature: .*$feature" $tmp.1 > /dev/null
            then : OK
            else
                [ "$qflag" = "yes" ] ||
                echo "           (Standard = $std - missing $feature)"
                tfail=1
            fi
        done
        if [ $tfail = 0 ]
        then
            echo "== PASS == (Standard = $std)"
            : $((pass++))
        else
            echo "!! FAIL !! (Standard = $std)"
            : $((fail++))
        fi
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
