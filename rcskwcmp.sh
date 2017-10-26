#!/bin/bash
#
# @(#)$Id: rcskwcmp.sh,v 1.1 2017/10/26 21:45:35 jleffler Exp $
#
# Compare two files, ignoring differences in RCS keyword expansions

# Gyrations deemed necessary when scc was ported to a machine with only
# Bash 3.2.25, which does not have process substitution <(cmd file) support.
RCSKW="${RCSKWREDUCE:-rcskwreduce}"

case "$#" in
(2) : OK;;
(*) echo "Usage: rcs_kw_cmp file1 file2" >&2
    exit 1;;
esac

# Preferred notation:
# exec cmp -s <($RCSKW "$1") <($RCSKW "$2")

# Safer to use tmp=$(mktemp "{$TMPDIR:-/tmp}/rcskwcmp.XXXXXX")
# But is it as portable?  Hard to say!  HP-UX?  AIX?  Solaris?
# Linux, macOS (and Mac OS X) are OK, even a fairly long way back.
tmp="${TMPDIR:-/tmp}/rcskwcmp.$$"
trap 'rm -f $tmp.?; exit 1' 0 1 2 3 13 15

$RCSKW "$1" > "$tmp.1"
$RCSKW "$2" > "$tmp.2"
cmp -s "$tmp.1" "$tmp.2"
status=$?
rm -f "$tmp".?
trap 0
exit $status
