//pxit-encoder - software to convert an input file into a directory of 
//images that represent the information in the file.

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
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

/* pxit-encoder converts an arbitrary file into a sequence of numbered
 * graphics image files in TARGA format.
 *
 * Specifications:
 *  image resolution: 720x480  pixels (width x height)
 *  cell size: 16x16.  cells completely cover screen
 *  looks like 45x30 grid of "checkerboard squares"
 *  squares (called color cells) are either red, white, blue, or green.
 * 
 * Capacity:
 *  there are 1350 color cells each representing two bits
 *  there are 1350/4 = 337 bytes per image
 * 
 * Format:
 *    0 -   2 (  3 bytes): file length
 *    3 -   4 (  2 bytes): frame number
 *    5 - 332 (328 bytes): data
 *  333 - 336 (  4 bytes): checksum
 * 
 * I/O:
 *  path to input file supplied on command line
 *  images are created in the same directory as the input file.
 * 
 * pxit-endoder is used for production.
 * 
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "checksum.h"
#include "TargaImage.h"
#include "pxit-parms.h"



int *frame; //holds a bitmap
void drawCell(int row_, int col_, int color);

int main(int argc, char *argv[]){

    //Validate inputs.  Expect only a path to the input file.
    if(argc != 2) {
        printf("\tUsage: %s <path to input file>\n",argv[0]);
        return 0;
    }

    printf("\t**********Welcome to pxit-encoder**********\n\n");  
    printf("\tConverting %s into image files\n\n",argv[1]);
    
    //Validate input. Can we access the file?
    char dir[200], base[200];
    strcpy(dir,dirname(argv[1]));          //returns string up to (but not including) final /
    strcpy(base,argv[1]+(strlen(dir)+1));  //base filename starts after the final /
    chdir(dir);
  

    FILE *fp=fopen(base,"r");
    int filesize;
    
    if(fp == NULL) {
        perror("fd");
        return 0;
    } else {
        fseek(fp, 0L, SEEK_END);
        filesize = ftell(fp);
        rewind(fp);
    }
    
    //Create a Targa object and get a pointer to its bitmap
	TargaImage *tga = new TargaImage(width, height);
    frame = (int *)tga->getFrame();
    
    //create an object that can compute error-correcting codes
	CheckSum *checksum = new CheckSum(packetSize);
    
    //convert filesize to three bytes
	unsigned char hi =  (unsigned char)(filesize >> 16);
	unsigned char med = (unsigned char)(filesize >> 8);
	unsigned char lo =  (unsigned char) filesize;
    

	//Compute the number of images as needed to encode the selected input file
	int framesNeeded = filesize / blockSize; 
	if (blockSize * framesNeeded < filesize) 
        framesNeeded++;  //The last frame will be partially filled

 	int frameNumber = 0;
	int blockSequence = 0;

    unsigned char packet[packetSize];  //buffer to hold packet we're building
    char pixelstream[packetSize * 4];  //holds color cell representation of packet
    
  	while(frameNumber < framesNeeded) {
		memset(packet, 0, packetSize);

		//write the file length Big Endian
		packet[0] = hi;
		packet[1] = med;
		packet[2] = lo;

		//now convert frameNumber into a two-byte offset.
		int indhi = (blockSequence >> 8) & 0xFF;
		int indlo = blockSequence & 0xFF;
		packet[3] = indhi;
		packet[4] = indlo;
        
		blockSequence++;  //prepare for next block

        //Read a data block from file and copy it to packet
        //following the header (5 bytes)
        int bytesRead = fread(packet+5,1,blockSize,fp);
  
        if(bytesRead < blockSize) { //We didn't get a full block
            if(feof(fp)) {
                //We got the last packet.  Pad it with zeros
                int firstzero = bytesRead + 5;  //header is 5 bytes long.
                for(int i=firstzero; i<packetSize; i++) packet[i] = 0;
            } else{
                printf("packet read error\n");
                return 0;
            }
        }
        
        //Compute checksum
        checksum->compute(packet, packetSize);
       
       
       /* At this point we have a complete date packet.  We now have to 
        * "paint a picture".  The packet is a sequence of bytes.  Each byte
        * is a sequence of four two-bit symbols.  The symbols are one digit
        * numbers in the Base 4 system: 00, 01, 10, 11.  The numerals we
        * use are actually colors: red, white, blue, and green, respectively.
        * We call this stream of colors a 'pixelstream'.
        */

		int cell = 0;
		for (int i = 0; i < packetSize; i++) {
            
            const int b1 = 4;		//  4s place
            const int b2 = b1 * 4;	// 16s place
            const int b3 = b2 * 4;	// 64s place
			int base4digit[4];

            //start with a byte
			int byte = packet[i];

			//convert the byte into four base 4 base4digita
			base4digit[0] = byte / b3;
			byte -= b3*base4digit[0];

			base4digit[1] = byte / b2;
			byte -= b2*base4digit[1];

			base4digit[2] = byte / b1;
			byte -= b1*base4digit[2];

			base4digit[3] = byte;

			pixelstream[cell]	  = (unsigned char)base4digit[0];
			pixelstream[cell + 1] = (unsigned char)base4digit[1];
			pixelstream[cell + 2] = (unsigned char)base4digit[2];
			pixelstream[cell + 3] = (unsigned char)base4digit[3];

			cell += 4;
		}
        
        /* Now create the image by assigning colors to the squares based on the array 
         * pixelstream.  The image will display a 45x30 array of color cells.
         */

		cell = 0;
		
        for (int cy = 0; cy <= 29; cy++) {              //for every row
            for (int cx = 0; cx <= 44; cx++) {          //for every colums
                drawCell(cy, cx, pixelstream[cell]);    //draw a solid square
                cell++;
            }
        }
		
        //form a filename using frame number and save the image.
        char tmp[256];
        sprintf(tmp,"%s-%02d.tga",base,frameNumber);
        tga->writeFile(tmp);

        frameNumber++;  //prepare for nexe frame.
            
    }//end while
    
    printf("\t%d images produced. File conversion complete.\n\n",frameNumber);
    return 0;
}

void drawCell(int row_, int col_, int value) {

	//Note: (row_, col_) refer to rows and columns in the rectangular array of cells.
	//      (row , col ) refer to pixel coordinates within an image.

    //convert cell array coordinates to image pixel coordinates
	int row = row_*cellsize;
	int col = col_*cellsize;

    int pxvalue;  //pixel color expressed as an integer between 0 and 3.
    
	switch (value) {  //2-bit value that the cell should represent
        case 0: pxvalue = 0xFFFF0000; break; //red
        case 1: pxvalue = 0xFFFFFFFF; break; //white
        case 2: pxvalue = 0xFF0000FF; break; //blue
        case 3: pxvalue = 0xFF00FF00; break; //green
        default:
                //This should never happen.
                pxvalue = 0xFF000000;        //black
	}

    //Color in the cell
	for (int r = row; r<row + cellsize; r++) {
		for (int c = col; c<col + cellsize; c++) {
			*(frame + r*width + c) = pxvalue;
		}
	}
}

