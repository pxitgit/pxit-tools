#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "checksum.h"
#include "pxit-parms.h"

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();
    int processImage(int *frame);
    void getPixelstream(int *frame, char *pixelstream);  
private:
    //variables
    bool         *BlockFlags;
    int           blocksNeeded;
    CheckSum     *checksum;
    //char          directory[200];
    bool          fileComplete=false;
    int           filelength;
    bool          gotFirstFrame=false;
    int           lastBlockSize;
    int           nBlocksFound;
    FILE         *outputFile;
    char          outputfname[200];
    unsigned char packet[packetSize];
    char          pixelstream[45*30];
    int           previous=0;
    int           sequence;  

    //methods
    void getDataPacket(char *pixelstream, unsigned char* packet);
    void resetDecoder();
};


#endif // IMAGEPROCESSOR_H
