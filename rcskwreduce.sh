#!/bin/sh
#
# @(#)$Id: rcskwreduce.sh,v 1.2 2017/10/11 21:07:52 jleffler Exp $
#
# Normalize (reduce) expanded RCS keywords to minimal form.

exec ${PERL:-perl} -p -e 's/\$(Author|Date|Header|Id|Locker|Log|Name|RCSfile|Revision|Source|State): [^\$]* \$/\$$1\$/g' "$@"

