#!/bin/sh
#
# @(#)$Id|
#
# Normalize (reduce) expanded RCS keywords to minimal form.

exec ${PERL:-perl} -p -e 's/\$(Author|Date|Header|Id|Locker|Log|Name|RCSfile|Revision|Source|State): [^\$]* \$/\$$1\$/g' "$@"

