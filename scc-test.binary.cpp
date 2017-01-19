/*
@(#)File:           $RCSfile: scc-test.binary.cpp,v $
@(#)Version:        $Revision: 1.3 $
@(#)Last changed:   $Date: 2014/07/16 07:07:50 $
@(#)Purpose:        Test SCC on C++14 binary numbers
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2014
@(#)Product:        SCC Version 6.50 (2016-06-12)
*/

int i = 0b0100;
int j = 0b1000'1000;
int k = 0b1010'0101'1111'0000;
int l = 0B1011;
int m = 0B1'0'1'1;  // Not sensible, but valid
