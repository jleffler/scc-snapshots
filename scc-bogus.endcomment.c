/*
** @(#)$Id: scc-bogus.endcomment.c,v 1.1 2019/01/26 07:33:27 jonathanleffler Exp $
**
** Test SCC handling of * in general program text followed by a
** backslash-newline (bsnl) sequence and then / (a bogus end comment
** marker).
*/

/* The next comment is legitimate - well, sort of legitimate */
int x; /\
\
\
*\ comment \
*\
/

/* This next end comment marker is not legitimate */
int y;   *\
\
\
/

/* There are two end comment markers on the next line; one complaint! */
int z; */ gibberish */

/* There are two end comment markers in the following block */
int a; *\
/ comment mismanaged /
*/

/* EOF */
