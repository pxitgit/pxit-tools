/*
Copyright (c) 2020, Frank J. LoPinto

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

#include <stdlib.h> //for exit()
#include "ImageProcessor.h"
#include "TargaImage.h"

void showSamplePoints(int *frame);
ImageProcessor::ImageProcessor() {  //Convert stream of images into a file
    checksum = new CheckSum(packetSize);
}

ImageProcessor::~ImageProcessor() {delete checksum;}

void ImageProcessor::getDataPacket(char *pixelstream, unsigned char* packet) {
	int cellIndex = 0;
	for (int byteIndex = 0; byteIndex < packetSize; byteIndex++) {
		cellIndex = byteIndex * 4;
		int byte = 0;
		byte  = pixelstream[cellIndex];
		byte <<= 2;
		byte |= pixelstream[cellIndex + 1];
		byte <<= 2;
		byte |= pixelstream[cellIndex + 2];
		byte <<= 2;
		byte |= pixelstream[cellIndex + 3];

		packet[byteIndex] = byte;
	}
}

void ImageProcessor::getPixelstream(int *frame, char *pixelstream) {
    
    //getPixelstream() examines a frame and samples 45x30 pixel values.  'offset' defaults to 0.
    //If 'offset' is -1 then the software samples pixels one row above the default value.
    //This allows us to separate distinct images represented by different interlaces.  In other
    //words each captured frame has complete information from two source frames.
    
    int nrows = height/cellsize;
    int ncols = width /cellsize;
    int cnt = 0;
    for(int r=0;r<nrows;r++) {
        for(int c=0;c<ncols;c++) {
            
            //sample the top-left corner of cell
            //NO take the centers
            int x = c * cellsize;
            int y = r * cellsize;
            
            x += cellsize/2;
            y += cellsize/2;


            int pixelcolor = *(frame + width*y  + x);
            
            //extract red, green, and blue components
            int red = (pixelcolor >> 16) & 0xFF;
            int grn = (pixelcolor >>  8) & 0xFF;
            int blu = (pixelcolor      ) & 0xFF;
          
            //compute colors assuming errors
            if(red>180 && grn>180 && blu>180) pixelstream[cnt] = 1;
            else if(red>grn && red>blu)  pixelstream[cnt]=0;
            else if(grn>red && grn>blu)  pixelstream[cnt]=3;
            else  pixelstream[cnt]=2;
           
            cnt++;            
        }
    }
}

//processImage return codes: 0 <- normal return (includes bad checksums)
//                          -1 <- bad checksum following valid packet
//                           1 <- file complete

int ImageProcessor::processImage(int *frame) {
    //convert frame (bitmap) into a stream of 2-bit symbols
    getPixelstream(frame,pixelstream);
    
    //convert stream of symbols into stream of bytes
    getDataPacket(pixelstream, packet);
    
    //reject packets without valid checksums
    if(!checksum->verify(packet,packetSize)) {
        if(gotFirstFrame) return -1;
        else return 0;
    }
    
    //compare file length in header with previous value
	filelength = packet[0];
	filelength <<= 8;
	filelength |= packet[1];
	filelength <<= 8;
	filelength |= packet[2];
    
    int sequence;
	sequence = packet[3];
	sequence <<= 8;
	sequence |= packet[4];

    
    //Did the file length change?
	if(filelength != previous && previous != 0) 
        resetDecoder();     //yes. prepare to decode a new file
    else if(fileComplete)   
        return 0;           //no. ignore this packet.

    //Is this the first packet we've seen from the file?
    if(!gotFirstFrame) {
        gotFirstFrame = true;

		//Compute number of data blocks (packets) needed and the size of the last one.
		blocksNeeded = filelength / blockSize;
		lastBlockSize = filelength - blocksNeeded*blockSize;
		if (lastBlockSize) blocksNeeded++;  //last block is partially filled	
        
        //create an output file. use time to create filename
        time_t     now;
        struct tm  ts;

        // Get current time
        time(&now);

        // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
        ts = *localtime(&now);
        strftime(outputfname, sizeof(outputfname), "%Y-%m-%d_%H:%M:%S_%Z.7z", &ts);

        outputFile = fopen(outputfname, "wb");  //note .7z extension.
        if(outputFile == NULL) perror("fopen");
		BlockFlags = new bool[blocksNeeded];	//bool array for each block we'll need
        
		for (int i = 0; i < blocksNeeded; i++)
			BlockFlags[i] = false;			    //no blocks have been processed yet
    }
    

    //have we seen this sequence number before?
    if (!BlockFlags[sequence]) {
        
        //No, this is a new one. Update records
        BlockFlags[sequence] = true;	//We just got a new block
        nBlocksFound++;

        //Seek to location based on sequence number found.
        fseek(outputFile, sequence*blockSize, SEEK_SET);

        //Compute number of bytes to copy
        int bytesToCopy = blockSize;
        if (sequence + 1 == blocksNeeded) 
            bytesToCopy = lastBlockSize;

        //Copy user data to the file
        fwrite((char *)&packet[headerSize], 1, bytesToCopy, outputFile);
    }

    //Have we gotten the entire file?
    if (nBlocksFound == blocksNeeded) {
        fclose(outputFile);
        fileComplete = true;
        filelength = 0; //so we can receive another file, possibly the same one
        printf("File Transfer Complete: %s\n",outputfname);
    }
    
    return 1;
}

void ImageProcessor::resetDecoder() {
    gotFirstFrame = false;
    nBlocksFound = 0;
    previous = filelength;
    fileComplete = false;
}




