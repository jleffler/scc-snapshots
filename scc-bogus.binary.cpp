/*
@(#)File:           $RCSfile: scc-bogus.binary.cpp,v $
@(#)Version:        $Revision: 1.2 $
@(#)Last changed:   $Date: 2014/08/21 07:13:51 $
@(#)Purpose:        Test SCC on bogus C++14 binary numbers
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2014
@(#)Product:        SCC Version 8.0.2 (2022-05-29)
*/

int i = 0b'0100;                // Quote without digit before
int j = 0b0100';                // Quote without digit after
int k = 0b2;                    // Non-binary digit
int l = 0b1'20'0101'1111'0000;  // Non-binary digit
int m = 0b1'02;                 // Non-binary digit
int n = 0B10''11;               // Successive quotes
