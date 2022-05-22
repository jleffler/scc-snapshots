/*
@(#)File:           $RCSfile: scc-test.example3.c,v $
@(#)Version:        $Revision: 1.1 $
@(#)Last changed:   $Date: 2016/05/04 06:04:40 $
@(#)Purpose:        Test handling of C11 (and C++11) Unicode characters and strings.
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2016
@(#)Product:        SCC Version 8.0.1 (2022-05-21)
*/

#include <wchar.h>  /* wchar_t */
//#include <uchar.h>
#include "uchar.h"  /* int16_t, int32_t */

int main(void)
{
    wchar_t  wc   = L'x';
    char16_t uc   = u'x';
    char32_t Uc   = U'x';
    wchar_t  ws[] = L"x";
    char16_t us[] = u"x";
    char32_t Us[] = U"x";
    return wc + uc + Uc + ws[0] + us[0] + Us[0];
s
