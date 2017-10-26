/*
@(#)File:           $RCSfile: scc-test.numpunct.cpp,v $
@(#)Version:        $Revision: 1.2 $
@(#)Last changed:   $Date: 2015/06/19 03:33:26 $
@(#)Purpose:        Test SCC on numbers with C++14 punctuation
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2014-15
@(#)Product:        SCC Version 6.80 (2017-10-26)
*/

float f1 = 0x12'34'45p-12;
float f2 = 0X12'34'45P-12;
int b1 = 0b0000'1010;
int b2 = 0B1111'1100'1011'0100;

int x1 = 0xA'B'C'D;
int x2 = 0xFFEE'DDCCLL;
int o1 = 011'101'111;
int o2 = 01'1'0'1'1;
int o3 = 0'0'1'0'1'1;
int d1 = 123'456;
int d2 = 9'8'7'6'5'4'3;

void set_b2(void)
{
    b2 = 0B1111'1100'1011,0100;     // Comma operator allowed!
}
