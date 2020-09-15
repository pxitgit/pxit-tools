//Checksum.cpp - implements Galois Field - based checksum algorithm

/*Copyright (c) 2020, Frank J. LoPinto

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE*/


#include <stdio.h>
#include <string.h>
#include "checksum.h"

unsigned char CheckSum::add(const unsigned char a, const unsigned char b) {
	return a^b;
}
unsigned char CheckSum::mult(const unsigned char a, const unsigned char b) {
	if(a==0 || b==0) 
		return 0;
	else
		return alpha[(alphaLog[a]+alphaLog[b])%255];
}

CheckSum::CheckSum(int pktSize) {

	//Allocate the buffer.  
        //Note that pktSize includes the header and checksum fields.
	buffer = new unsigned char[pktSize];

	//Generate the Galois field GF8.  Fill in the first eight by hand.
	alpha[0] = 0x01;	alphaLog[0x01] = 0;
	alpha[1] = 0x02;	alphaLog[0x02] = 1;
	alpha[2] = 0x04;	alphaLog[0x04] = 2;
	alpha[3] = 0x08;	alphaLog[0x08] = 3;
	alpha[4] = 0x10;	alphaLog[0x10] = 4;
	alpha[5] = 0x20;	alphaLog[0x20] = 5;
	alpha[6] = 0x40;	alphaLog[0x40] = 6;
	alpha[7] = 0x80;	alphaLog[0x80] = 7;

	//Then generate the rest of the non-zero elements.  
    //The field generator polynomial
	//is written g(x) = x**8 + x**7 + x**2 + x + 1.
	//
	//It seems reasonable to read this as x**8 = x**7 + x**2 + x + 1
	unsigned char g = 0x87;
	for(int i = 8; i<255; i++)    {
		alpha[i] = alpha[i-1] << 1;
		if(alpha[i-1] & 0x80) alpha[i] ^= g;
		alphaLog[alpha[i]] = i;
	}

	/*
		Create the code generator polynomial.
		p(x) = (x + e1)(x + e2)(x + e3)(x + e4)  ei are elements of GF8.
		p(x) = (x**2 + g1 x + g2) (x**2 + g3 x + g4)
			
			  where: g1 = e1 + e2
					 g2 = e1 * e2
					 g3 = e3 + e4
					 g4 = e3 * e4

		p(x) = x**4 + (g1 + g3) x**3 + (g4 + g2 + g1*g3) x**2 + 
                                               (g1*g4 + g3*g2) x + g2*g4
		p(x) = x**4 + p3 x**3 + p2 x**2 + p1 x +p0

			  where: p3 = g1 + g3
					 p2 = g4 + g2 + g1*g3
					 p1 = g1*g4 + g3*g2
					 p0 = g2*g4
	*/


	unsigned char e1 = 0x09, e2 = 0x11, e3 = 0x20, e4 = 0x01; 
                           //any four elements would do.

	unsigned char g1 = e1 ^ e2;
	unsigned char g2 = mult(e1, e2);
	unsigned char g3 = e3 ^ e4;
	unsigned char g4 = mult(e3, e4);

	p[3] = g1 ^ g3;
	p[2] = g4 ^ g2 ^ mult(g1, g3);
	p[1] = mult(g1,g4) ^ mult(g3, g2);
	p[0] = mult(g2, g4);

	//Create look up table by multiplying p(x) by every possible 
    //coefficient.  Zero is included for completeness.
	//The checksum algorithm just skips null coefficients.
	for(int msb = 0; msb <= 0xFF; msb++)
		for(int i=0;i<4;i++) lut[msb][i] = mult(p[i],msb);

}//end ctor


void CheckSum::compute(unsigned char *pkt, const int pktLen) {

	//Computes checksum using Galois Field arithmetic.  Each byte of the 
	//packet is interpreted as a symbol in the finite field GF8.

	//Interpret pkt as a polynomial in descending powers of x.  So pkt[0] 
	//holds the coefficient of the largest power of x used.

	//Note: the caller must allocate buffers for pkt.	  
	memcpy(buffer,pkt,pktLen-4);	//Don't destroy the caller's buffer. 
	memset(buffer+pktLen-4,0,4);	//Make sure the checksum bytes are 
                                //initially zero.


	//The set of symbols form the coefficients of a polynomial m(x).  The 
	//highest-order coefficient is in pkt[0].  Assume for the moment that 
	//pkt[0] is non-zero.  Then the degree of m(x) is pktLen-1.

	//We multiply m(x) by x**4 to produce a polynomial of degree pktLen+3.  
	//The coefficients of x**3, x**2, x**1, and the constant are all zero. 
	//Divide m(x) by p(x) (which is a polynomial of degree 4).  This will 
	//produce a remainder that is, in general, a polynomial of degree 3.   
	//The four coefficients of the remainder constitute the checksum.

	int degree = pktLen - 1;
	int ndx = 0;  //start by pointing to the highest order coefficient

	//Execute a loop that reduces the degree of m(x) but one for each pass.

	while(degree >= 4) {
		
		unsigned char msb = buffer[ndx];  //most significant byte 
                                         //of the polynomial

		//Note: if msb is zero we don't have to execute these four 
		//		steps.  But we do have to test for zero every 
		//		time.  Since we'll typically be sending 
        //      compressed files, we shouldn't expect long 
        //      strings of zeros (or anything else).
                
		buffer[ndx+1] ^= lut[msb][3];
		buffer[ndx+2] ^= lut[msb][2];
		buffer[ndx+3] ^= lut[msb][1];
		buffer[ndx+4] ^= lut[msb][0];


		//Note: the powers of x decrease as ndx increases.
		degree--;
		ndx++;
	}

	//Copy the remaining terms into the check sum array.
	//Remember that buffer(x) = pkt(x) * x**4
 

	unsigned char *ptr1, *ptr2;
	ptr1 = &pkt[ndx];//-4];
	ptr2 = &buffer[ndx];

	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;

}//end compute

bool CheckSum::verify(const unsigned char *pkt, int pktLen) {
	memcpy(buffer,pkt,pktLen);	//Don't destroy the caller's buffer. 
    
	//Make sure the packet is not all the same color.  If it is, then we're 
	//surely looking  as some background and not on encoded image data.
	unsigned char val;
	bool multiValued = false;

	for(int i=0; i<pktLen; i++)
		if(i==0) val = *buffer;
		else if(*(buffer+i) != val) {
			multiValued = true; break;
		}


	if(!multiValued) return false;

	
	int degree = pktLen + 3;
	int ndx = 0;  //point to the highest order coefficient in m(x)

	while(degree >= 4) {
		
		unsigned char msb = buffer[ndx];  //most significant byte 

		buffer[ndx+1] ^= lut[msb][3];
		buffer[ndx+2] ^= lut[msb][2];
		buffer[ndx+3] ^= lut[msb][1];
		buffer[ndx+4] ^= lut[msb][0];

		//Note: the powers of x decrease as ndx increases.
		degree--;
		ndx++;
	}

	unsigned char cs1, cs2, cs3, cs4;
	ndx -= 4;
	cs1 = buffer[ndx];
	cs2 = buffer[ndx+1];
	cs3 = buffer[ndx+2];
	cs4 = buffer[ndx+3];
	if(cs1 || cs2 || cs3 || cs4) {
		//printf("checksum false\n");
		return false;
	} else {
		//printf("checksum true\n");
		return true;
	}
}//end verify
	
void CheckSum::PrintTables(const char *dumpFile) {
	//Print file displaying log and antilog tables
	FILE *fp = fopen(dumpFile,"wb");
	fprintf(fp,"Galois Field Generator Program v0.3 (October, 2005)");
	fprintf(fp,"\n\n");
	fprintf(fp,"Exponent\t\tSymbol\n");
	for(int i=0;i<=255;i++) 
		fprintf(fp,"alpha[%d]\t\t%x\n",i,alpha[i]);

	fprintf(fp,"\n\nSymbol\t\tLogarithm\n");
	for(int i=1;i<=0xFF;i++)
		fprintf(fp,"%x\t\t\t%d\n",i,alphaLog[i]);

	fprintf(fp,"\nNote: The log of 0x00 is not defined.\n");
	fclose(fp);
}//end PrintTables