/* imageprocessor - examine a stream of images and reconstruct files.
 * 
 */
#include <stdio.h> 
#include <string.h>
#include "checksum.h"

class ImageProcessor {
public:
    ImageProcessor(char *dir);
    void processImage(int *frame);
private:
    //constants
    const int width  = 720;
    const int height = 480;
    const int cellsize   =  16; //cells are 16x16 pixels.
    const static int packetSize = 337; //size of packet (bytes)
    const int blockSize  = 328; //size of data field
    
    //variables
    char directory[200];
    int sequence;  
    int filelength;
    int previous=0;
    int nBlocksFound;
    int blocksNeeded;
    bool gotFirstFrame=false;
    bool fileComplete=false;
    char pixelstream[45*30];// width/cellsize = 45
    unsigned char packet[packetSize];
    CheckSum *checksum;
    
    //methods
    void getDataPacket(char *pixelstream, unsigned char* packet);
    void getPixelstream(int *frame, char *pixelstream);  
    void resetDecoder();
};


ImageProcessor::ImageProcessor(char *dir) {
    strcpy(directory,dir);
    checksum = new CheckSum(packetSize);
}



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
    
    int nrows = height/cellsize;
    int ncols = width /cellsize;
    int cnt = 0;
    for(int r=0;r<nrows;r++) {
        for(int c=0;c<ncols;c++) {
            
            //sample the top-left corner of cell
            int x = c * cellsize;
            int y = r * cellsize;

            int pixelcolor = *(frame + width*y + x);
            
            //extract red, green, and blue components
            int red = (pixelcolor >> 16) & 0xFF;
            int grn = (pixelcolor >>  8) & 0xFF;
            int blu = (pixelcolor      ) & 0xFF;
            
            /* Colors represent numbers:
             * 0 = red
             * 1 = white
             * 2 = blue
             * 3 = green
             * 
            */
            if(red==255 && grn==255 && blu==255) pixelstream[cnt] = 1;
            else if(red==255)                    pixelstream[cnt] = 0;
            else if(blu==255)                    pixelstream[cnt] = 2;
            else                                 pixelstream[cnt] = 3;
            
            cnt++;            
        }
    }
}
 
void ImageProcessor::processImage(int *frame) {
    //convert frame (bitmap) into a stream of 2-bit symbols
    getPixelstream(frame,pixelstream);
    
    //convert stream of symbols into stream of bytes
    getDataPacket(pixelstream, packet);
    
    //reject packets without valid checksums
    if(!checksum->verify(packet,packetSize)) return; 
    
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
        resetDecoder();
    
    //Is this the first packet we've seen from the file?
    if(!gotFirstFrame) {
        gotFirstFrame = true;
 
		//Compute number of data blocks (packets) needed and the size of the last one.
		blocksNeeded = filelength / blockSize;
		int lastBlockSize = filelength - blocksNeeded*blockSize;
		if (lastBlockSize) blocksNeeded++;  //last block is partially filled	
    }
}


void ImageProcessor::resetDecoder() {
    gotFirstFrame = false;
    nBlocksFound = 0;
    previous = filelength;
    fileComplete = false;
}
    
#if 0
		//Open a file for outputting.  We expect the received file is compressed.
		char pathbuffer[512];
		char ext[5];
		strcpy(ext, ".7z");
		strcpy(pathbuffer, OutputDirectory);

		//Base filename on time of day
		char buf[100];
		strcpy(buf, logger->timestring());
		sprintf(OutputFileName, "%s%s%s", pathbuffer, buf, ext);
		outputFile = fopen(OutputFileName, "wb");
		sprintf(buf, "Create %s\n", OutputFileName);
		logger->logthis(buf);

		BlockFlags = new bool[blocksNeeded];		//Get a new array for the new file
		for (int i = 0; i < blocksNeeded; i++)
			BlockFlags[i] = false;			        //Initialize the new array to nothing found.
	}

	//Have we seen this data packet before?
	if (!BlockFlags[sequence]) {

		//No, this is a new one. Update records
		BlockFlags[sequence] = true;	//We just got a new block
		nBlocksFound++;
		nFrames++;

		//Seek to location based on sequence number found.
		fseek(outputFile, sequence*blockSize, FILE_BEGIN);

		//Compute number of bytes to copy
		int bytesToCopy = blockSize;
		if (sequence + 1 == blocksNeeded) 
			bytesToCopy = lastBlockSize;

		//Copy user data to the file
		fwrite((char *)&packet[headerSize], 1, bytesToCopy, outputFile);
		nBytes += bytesToCopy;

		//Have we gotten the entire file?
		if (nBlocksFound == blocksNeeded) {
			fclose(outputFile);
			fileComplete = true;
			//stalled = false;
			char msg[100];
			sprintf(msg, "Transfer complete. %d frames, %d errors, %d gaps\n", nFrames, nErrors, nGaps);
			logger->logthis(msg);

			updateIndex(OutputFileName);
			filelength = 0; //so we can receive another file, possibly the same one
		}
	}
    
    

}
	
#endif	
    
