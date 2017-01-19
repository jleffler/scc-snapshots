/*
@(#)File:           $RCSfile: filter.h,v $
@(#)Version:        $Revision: 2008.1 $
@(#)Last changed:   $Date: 2008/02/11 07:39:08 $
@(#)Purpose:        Header for filter functions
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1993,1995-98,2003-04,2006,2008
@(#)Product:        :PRODUCT:
*/

/*TABSTOP=4*/

#ifndef FILTER_H
#define FILTER_H

#ifdef MAIN_PROGRAM
#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
const char jlss_id_filter_h[] = "@(#)$Id: filter.h,v 2008.1 2008/02/11 07:39:08 jleffler Exp $";
#endif /* lint */
#endif /* MAIN_PROGRAM */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

/* Classic mode - not recommended for new programs */
/* Non-const file name; no error status feedback */
/* Source: filter.c */
typedef void (*ClassicFilter)(FILE *ifp, char *fn);
extern void filter(int argc, char **argv, int optnum, ClassicFilter function);

/* Modern mode 1 - without output file specified */
/* Source: stdfilter.c */
typedef int (*StdoutFilter)(FILE *ifp, const char *fn);
extern int filter_stdout(int argc, char **argv, int optnum, StdoutFilter function);

/* Modern mode 2 - with output file specified */
/* NB: OutputFilter is compatible with AFF code */
/* Source: outfilter.c */
typedef int (*OutputFilter)(FILE *ifp, const char *fn, FILE *ofp);
extern int filter_output(int argc, char **argv, int optnum, OutputFilter function);

/* Standard I/O error check code */
/* Used internally by filter_stdout() and filter_output(). */
/* Not normally used by client programs. */
/* Source: filterio.c */
extern int filter_io_check(FILE *ifp, const char *ifn, FILE *ofp);

#endif /* FILTER_H */
