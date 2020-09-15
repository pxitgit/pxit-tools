#ifndef __Checksum_h__
#define __Checksum_h__

const int MAXMSG = 4000;  //What should this value be?

class CheckSum {
public:
	CheckSum(int pktSize);
	void compute(unsigned char *pkt, const int pktLen);
	bool verify (const unsigned char *pkt, int pktLen);

	void PrintTables(const char *txt);
	
protected:
	unsigned char p[4];				//Represents p(x) without the leading x**4 term
	unsigned char alpha[255];	    //Elements of GF8 expressed as powers of alpha.
	unsigned char alphaLog[255];	//Logarithms to the base alpha of 8-bit numbers.
	unsigned char *buffer;
	unsigned char lut[255][4];

	unsigned char  add(const unsigned char a, const unsigned char b);
	unsigned char mult(const unsigned char a, const unsigned char b);

};

#endif