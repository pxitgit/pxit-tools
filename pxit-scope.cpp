//pxit-scope - program to diagnose captured PXIT images

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

/* pxit-scope is used to split an interlaced image into two separate images.
 * Raw information about the RGB colors at sampled pixel locations is printed.
 * 
 * The algorithm converts raw colors into pure red, white, blue, or green.  It
 * creates a full image with the corrected colors.  Raw colors that cannot be
 * so classified are displayed in black.
 * 
 * The program expects to get a TARGA file as an input.  It bases the names of
 * its output files on the input filename.
 * 
 * pxit-scope is used for debugging.
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include "TargaImage.h"

// Constants describe a 720x480 image displaying a 45x30 array of 16x16 color cells.
const int cellsize = 16;
const int width = 720;
const int height = 480;

// Function prototypes 
void drawCell(int row_, int col_, char value,int *frame);
void showSamplePoints(int *frame, int *sp1, int *sp2);
char computeColor(int pixelcolor);
 
int main(int argc, char *argv[]){
    
    if(argc != 2) {
        printf("Usage: %s <input file>\n",argv[0]);
        return 0;
    }
    
    char ifname[200];       //input filename 
    char ofname[200];       //output filenames based on basename
    
    strcpy(ifname,argv[1]); //save input argument
    char *ptr = ifname + strlen(ifname)-4;  //point to 4 chars from end

    if(strcmp(ptr, ".tga")) {
        printf("Error. Input file should end in '.tga' \n");
        return -1;
    }
    
    //Make sure we can open the file.
    int fd = open(ifname,O_RDONLY);
    if(fd == -1) {
        perror("open");
        return -1;
    }
    
    printf("\t**********Welcome to pxit-scope**********\n\n");  
    printf("Analyze image in %s\n\n",ifname);

    //Modify ifname by ending the string at the '.'tga
    *ptr = 0;
    
    //Open a file to hold text output.
    strcpy(ofname,ifname);
    strcat(ofname,"-data.txt");
    
    FILE *data = fopen(ofname,"w");
    if(!data) {
        perror("open data file");
        return -1;
    }
    printf("Created %s\n",ofname);   
    
    //Instantiate an object to manipulate TARGA images
    TargaImage *tga = new TargaImage(argv[1],720,480);
    int *frame = (int *)tga->getFrame();
    int sp1[45*30], sp2[45*30];  //RGB colors at sampling points
    strcpy(ofname,ifname);
    strcat(ofname,"-annotated.tga");
 
    //Save image with sample points indicated.
    showSamplePoints(frame, sp1, sp2);
    tga->writeFile(ofname);
    printf("Created %s\n",ofname);
    
    //Create an image using sample points 1
    int cell = 0;
    for (int celrow= 0; celrow <= 29; celrow++) {
        for (int celcol = 0; celcol <= 44; celcol++) { //loop over cell numbers
            drawCell(celrow,celcol,computeColor(sp1[cell]),frame);
            fprintf(data,"row %d, col %d: z1 = %x, z2 = %x\n",celrow,celcol,sp1[cell],sp2[cell]);
            cell++;
        }
    }
    strcpy(ofname,ifname);
    strcat(ofname,"-field2.tga");   //Field holds second of two source frames
    tga->writeFile(ofname);
    printf("Created %s\n",ofname);   
    
    //Create an image using sample points 2
    cell = 0;
    for (int celrow= 0; celrow <= 29; celrow++) {
        for (int celcol = 0; celcol <= 44; celcol++) { //loop over cell numbers
            drawCell(celrow,celcol,computeColor(sp2[cell]),frame);
            cell++;
        } 
    }    
    strcpy(ofname,ifname);
    strcat(ofname,"-field1.tga");    //Field holds first of two source frames
    tga->writeFile(ofname);
    printf("Created %s\n",ofname);
    return 0;
}

 void drawCell(int row_, int col_, char value, int *frame) {

	//Note: (row_, col_) refer to rows and columns in the rectangular array of cells.
	//      (row , col ) refer to pixel coordinates within an image.
    //
    //NOTE: value ranges from 0 - 3.  It is the value of the RGB color.
    
	int row = row_*cellsize;
	int col = col_*cellsize;
    int pxvalue;
    
    switch (value) {  //2-bit value that the cell should represent
        case 0: pxvalue = 0xFFFF0000; break; //red
        case 1: pxvalue = 0xFFFFFFFF; break; //white
        case 2: pxvalue = 0xFF0000FF; break; //blue
        case 3: pxvalue = 0xFF00FF00; break; //green
        case 4: pxvalue = 0xFF000000;        //black
    }

	for (int r = row; r<row + cellsize; r++) {
		for (int c = col; c<col + cellsize; c++) {
			*(frame + r*width + c) = pxvalue;
		}
	}
}

char computeColor(int pixelcolor) {
    //extract red, green, and blue components
    int red = (pixelcolor >> 16) & 0xFF;
    int grn = (pixelcolor >>  8) & 0xFF;
    int blu = (pixelcolor      ) & 0xFF;
    
    //compute colors assuming errors
    if(red>128 && grn>128 && blu>128)             return 1;   //white
    else if(red>grn && red>blu && red>128)         return 0;   //red
    else if(blu>grn && blu>red && blu>128)         return 2;   //blue
    else if(grn>red && grn>blu && grn>128)         return 3;   //green
    else                                          return 4;   //error
}

void showSamplePoints(int *frame, int *sp1, int *sp2) {
    //Annotate the output. Put dots at encoding sample points.
    //Measurements taken from captureing test images and observing
    //color cell locations
    
    //sp1 and sp2 arrays are to be filledin with 32-bit color values
    
    int cell=0;

    for (int celrow= 0; celrow <= 29; celrow++) {
        for (int celcol = 0; celcol <= 44; celcol++) { //loop over cell numbers
            float x = celcol * cellsize;
            float y = celrow * cellsize; 
            
            //put mark near center of cell
            x += cellsize/2.0;
            y += cellsize/2.0;

            int ix, iy;
            ix = (int)(x + 0.5);
            iy = (int)(y + 0.5);
            
            int z1 = (iy+1)*width +ix;          
            *(sp2+cell) = *(frame + z1);
            *(frame + z1) = 0xFF000000;
    
            int z = iy * width + ix;            
            *(sp1+cell) = *(frame + z);
            *(frame + z)  = 0xFFffff00;
            
            cell++;
        }
    }
}