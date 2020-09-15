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

 /*
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
 * Author:
 *  Frank J. LoPinto
 *  July, 2020
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


const int b1 = 4;		//  4s place
const int b2 = b1 * 4;	// 16s place
const int b3 = b2 * 4;	// 64s place
const int b4 = b3 * 4;	//256s place

int *frame; //holds a bitmap image
void drawCell(int row_, int col_, int color);

int main(int argc, char *argv[]){

    //Validate inputs.  Expect only a path to the input file.
    if(argc != 2) {
        printf("\tUsage: %s <path to input file>\n",argv[0]);
        return 0;
    }

    printf("\t**********Welcome to pxit-encoder**********\n\n");  
    printf("\tProcessing %s\n",argv[1]);
    
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
    
    //Create a Targa object and get a pointer to its buffer
	TargaImage *tga = new TargaImage(width, height);
    frame = (int *)tga->getFrame();
    
    //convert filesize to three bytes
	unsigned char hi =  (unsigned char)(filesize >> 16);
	unsigned char med = (unsigned char)(filesize >> 8);
	unsigned char lo =  (unsigned char) filesize;
    
	//create an object that can compute error-correcting codes
	CheckSum *checksum = new CheckSum(packetSize);

	//Now create as many images as needed to encode the selected input file
	int framesNeeded = filesize / blockSize;  //global so it can be seen by the thread
	if (blockSize * framesNeeded < filesize) framesNeeded++;

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
		blockSequence++;

        //Read a data block from file and copy it to packet.
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
       
		//convert the bytes into a pixelstream
		int cell = 0;
		for (int i = 0; i < packetSize; i++) {

			int x = packet[i];  //get a byte

			//convert x to base 4
			int digit[4];
			digit[0] = x / b3;
			x -= b3*digit[0];

			digit[1] = x / b2;
			x -= b2*digit[1];

			digit[2] = x / b1;
			x -= b1*digit[2];

			digit[3] = x;

			pixelstream[cell]	  = (unsigned char)digit[0];
			pixelstream[cell + 1] = (unsigned char)digit[1];
			pixelstream[cell + 2] = (unsigned char)digit[2];
			pixelstream[cell + 3] = (unsigned char)digit[3];

			cell += 4;
		}
        
        //Now create the image by assigning colors to the squres
        //Loop through the areas.  For each color the cells according to the data in pixelstream
		cell = 0;
		
        for (int cy = 0; cy <= 29; cy++) {
            for (int cx = 0; cx <= 44; cx++) { //loop over cell numbers
                drawCell(cy, cx, pixelstream[cell]);
                cell++;
            }
        }
		
        char tmp[256];
        sprintf(tmp,"%s-%02d.tga",base,frameNumber);
        printf("create %s\n",tmp);
        tga->writeFile(tmp);

        //Get ready for next frame.
        frameNumber++;
            
    }//end while
    
    //For debugging hold the last frame for 3 seconds.
    
    printf("\t%d images produced. File conversion complete.\n\n",frameNumber);
    
    frameNumber--;
    
    //hold last frame for 3 seconds
    for(int i=0;i<90;i++) {
        char tmp[256];
        sprintf(tmp,"%s-%02d.tga",base,frameNumber++);
        tga->writeFile(tmp);

    }

    return 0;
}

void drawCell(int row_, int col_, int value) {

	//Note: (row_, col_) refer to rows and columns in the rectangular array of cells.
	//      (row , col ) refer to pixel coordinates within an image.

	int row = row_*cellsize;
	int col = col_*cellsize;

    int pxvalue;  //pixel color expressed as an integer.
    
	switch (value) {  //2-bit value that the cell should represent
        case 0: pxvalue = 0xFFFF0000; break; //red
        case 1: pxvalue = 0xFFFFFFFF; break; //white
        case 2: pxvalue = 0xFF0000FF; break; //blue
        case 3: pxvalue = 0xFF00FF00; break; //green
        default:
                pxvalue = 0xFF000000;        //black
	}

	for (int r = row; r<row + cellsize; r++) {
		for (int c = col; c<col + cellsize; c++) {
			*(frame + r*width + c) = pxvalue;
		}
	}
}

