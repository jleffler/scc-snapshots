/*
@(#)File:           $RCSfile: scc-test.hexfloat.cpp,v $
@(#)Version:        $Revision: 1.1 $
@(#)Last changed:   $Date: 2016/05/30 21:29:47 $
@(#)Purpose:        Hexadecimal floating point constants (C99 and later)
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2016
@(#)Product:        SCC Version 6.50 (2016-06-12)
*/

// There is evidence from G++ 6.1.0 that C++17 (C++1z) will support
// hexadecimal floating point.
// G++ accepts them unless invoked with an explicit standard
// (-std=c++14, for example).

const float xf01 = 0xA.BP-1;    // Testing tail comment behaviour (again)
const float xf02 = 0xABCP-1;
const float xf03 = 0x.ABP-1;
const float xf04 = 0XA.BP+2;
const float xf05 = 0XABCP+2;
const float xf06 = 0X.ABP+2;
const float xf07 = 0XA.BP36;
const float xf08 = 0XABCP36;
const float xf09 = 0X.ABP36;

const float xf11 = 0x8.7P-1;
const float xf12 = 0x87CP-1;
const float xf13 = 0x.87P-1;
const float xf14 = 0X8.7P+2;
const float xf15 = 0X87CP+2;
const float xf16 = 0X.87P+2;
const float xf17 = 0X8.7P36;
const float xf18 = 0X87CP36;
const float xf19 = 0X.87P36;

const float xf21 = 0xa.bp-1;
const float xf22 = 0xabcp-1;
const float xf23 = 0x.abp-1;
const float xf24 = 0xa.bp+2;
const float xf25 = 0xabcp+2;
const float xf26 = 0x.abp+2;
const float xf27 = 0xa.bp36;
const float xf28 = 0xabcp36;
const float xf29 = 0x.abp36;
