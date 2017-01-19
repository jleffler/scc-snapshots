/*
@(#)File:           $RCSfile: posixver.h,v $
@(#)Version:        $Revision: 1.1 $
@(#)Last changed:   $Date: 2010/08/29 00:27:48 $
@(#)Purpose:        Request appropriate POSIX and X/Open Support
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2010
@(#)Product:        SCC Version 5.05 (2012-01-23)
*/

/*TABSTOP=4*/

#ifndef JLSS_ID_POSIXVER_H
#define JLSS_ID_POSIXVER_H

/*
** Include this file before including system headers.  By default, with
** C99 support from the compiler, it requests POSIX 2001 support.  With
** C89 support only, it requests POSIX 1997 support.  Override the
** default behaviour by setting either _XOPEN_SOURCE or _POSIX_C_SOURCE.
*/

/* _XOPEN_SOURCE 700 is loosely equivalent to _POSIX_C_SOURCE 200809L */
/* _XOPEN_SOURCE 600 is loosely equivalent to _POSIX_C_SOURCE 200112L */
/* _XOPEN_SOURCE 500 is loosely equivalent to _POSIX_C_SOURCE 199506L */

#if !defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600   /* SUS v3, POSIX 1003.1 2004 (POSIX 2001 + Corrigenda) */
#else
#define _XOPEN_SOURCE 500   /* SUS v2, POSIX 1003.1 1997 */
#endif /* __STDC_VERSION__ */
#endif /* !_XOPEN_SOURCE && !_POSIX_C_SOURCE */

#endif /* JLSS_ID_POSIXVER_H */
