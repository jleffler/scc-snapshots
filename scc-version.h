/*
@(#)File:           $RCSfile: scc-version.h,v $
@(#)Version:        $Revision: 8.2 $
@(#)Last changed:   $Date: 2022/05/21 19:21:28 $
@(#)Purpose:        Version information for SCC
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2022
*/

#ifndef SCC_VERSION_H_INCLUDED
#define SCC_VERSION_H_INCLUDED

#ifdef MAIN_PROGRAM
#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
extern const char jlss_id_scc_version_h[];
const char jlss_id_scc_version_h[] = "@(#)$Id: scc-version.h,v 8.2 2022/05/21 19:21:28 jonathanleffler Exp $";
#endif /* lint */
#endif /* MAIN_PROGRAM */

static const char version_data[] = "@(#)$Version: 8.0.1 $ ($Datetime: 2022-05-21 19:21:01Z $)";
static const char cmdname_data[] = "@(#)SCC";
static const char * const version_info = &version_data[4];
static const char * const cmdname_info = &cmdname_data[4];

#endif /* SCC_VERSION_H_INCLUDED */

