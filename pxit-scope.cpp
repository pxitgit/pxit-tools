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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include "TargaImage.h"
#include "ImageProcessor.h"

void drawCell(int row_, int col_, int value,int *frame, bool rawcolor=false);
void showSamplePoints(int *frame, int *sp1, int *sp2);
void getstreams(int *frame, int *stream1, int *stream2);
void getpixelstreams(int *stream1, char *pixelstream1, int *stream2, char *pixelstream2);
char computeColor(int pixelcolor);
 
int main(int argc, char *argv[]){
    
    //Validate inputs. Make sure file exists and ends in .tga
    if(argc != 2) {
        printf("Usage: %s <input file>\n",argv[0]);
        return 0;
    }
    
    //Make sure the file ends in '.tga'
    char buf[200];
    strcpy(buf,argv[1]);
    char *ptr = buf + strlen(buf)-4;
    if(strcmp(ptr, ".tga")) {
        printf("Error. Input file should end in '.tga' \n");
        return -1;
    }
    
    //Open the selected file
    int fd = open(argv[1],O_RDONLY);
    if(fd == -1) {
        perror("open");
        return -1;
    }
    
    printf("\t**********Welcome to pxit-scope**********\n\n");  
    printf("Analyze image in %s\n",buf);


    char basename[200], filename[200]; 
    *(buf + (strlen(buf)-4)) = 0;
    strcpy(basename,buf);
    strcpy(filename,basename);
    strcat(filename,"-data.txt");
    
    FILE *data = fopen(filename,"w+");
    if(!data) {
        perror("open data file");
        return -1;
    }
    
    printf("opened %s\n",filename);
    
    
    //Instantiate an object to manipulate TARGA images
    TargaImage *tga = new TargaImage(argv[1],720,480);
    int *frame = (int *)tga->getFrame();
    int sp1[45*30], sp2[45*30];  //RGB colors at sampling points
    strcpy(filename,basename);
    strcat(filename,"annotated.tga");
 
    //Save image with sample points indicated.
    showSamplePoints(frame, sp1, sp2);
    tga->writeFile(filename);
    
    //Create an image using sample points 1
    int cell = 0;
    for (int celrow= 0; celrow <= 29; celrow++) {
        for (int celcol = 0; celcol <= 44; celcol++) { //loop over cell numbers
            drawCell(celrow,celcol,computeColor(sp1[cell]),frame);
            fprintf(data,"row %d, col %d: z1 = %x, z2 = %x\n",celrow,celcol,sp1[cell],sp2[cell]);
            cell++;
        }
    }
    strcpy(filename,basename);
    strcat(filename,"-spB.tga");   //Field holds second of two source frames
    tga->writeFile(filename);
    
   cell = 0;
    for (int celrow= 0; celrow <= 29; celrow++) {
        for (int celcol = 0; celcol <= 44; celcol++) { //loop over cell numbers
        
            //*(frame + celrow*width + celcol) = sp2[cell];
            drawCell(celrow,celcol,sp2[cell],frame, true);
          // printf("sp2 row %d, col %d: z1 = %x, z2 = %x\n",celrow,celcol,sp1[cell],sp2[cell]);
            cell++;
        } 
    }    
    strcpy(filename,basename);
    strcat(filename,"-spA.tga");    //Field holds first of two source frames
    tga->writeFile(filename);
    
    return 0;
}

 void drawCell(int row_, int col_, int value, int *frame, bool rawcolor) {

	//Note: (row_, col_) refer to rows and columns in the rectangular array of cells.
	//      (row , col ) refer to pixel coordinates within an image.
    //
    //NOTE: value ranges from 0 - 3.  It is the value of the RGB color.

	int row = row_*cellsize;
	int col = col_*cellsize;

    int pxvalue;  //pixel color expressed as an integer.
    pxvalue = value;
    if(!rawcolor) {
        switch (value) {  //2-bit value that the cell should represent
            case 0: pxvalue = 0xFFFF0000; break; //red
            case 1: pxvalue = 0xFFFFFFFF; break; //white
            case 2: pxvalue = 0xFF0000FF; break; //blue
            case 3: pxvalue = 0xFF00FF00; break; //green
            default:
                    pxvalue = 0xFF000000;        //black
        }
    
    }

	for (int r = row; r<row + cellsize; r++) {
		for (int c = col; c<col + cellsize; c++) {
			*(frame + r*width + c) = pxvalue;
		}
	}
}

 /*   int       stream1[45*30],      stream2[45*30];  //cell array 45x30.
    char pixelstream1[45*30], pixelstream2[45*30];  //value array
                                         
    getstreams(frame, stream1, stream2);  //get two streams of 32-bit RGB
    
    getpixelstreams(stream1, pixelstream1,stream2,pixelstream2); //convert to pixelstreams

    //Create new TargaImage objects to hold even and odd interlace 
    TargaImage *evenInterlace = new TargaImage(720,480);
    TargaImage *oddInterlace = new TargaImage(720,480);
    int *evenframe = (int *)evenInterlace->getFrame();
    int *oddframe = (int *)oddInterlace->getFrame();

    //Draw images using decoded colors of the sample points
    int cell = 0;
    for (int cy = 0; cy <= 29; cy++) {
        for (int cx = 0; cx <= 44; cx++) { //loop over cell numbers
        
            drawCell(cy, cx, pixelstream1[cell],oddframe);
            drawCell(cy, cx, pixelstream2[cell],evenframe);
            cell++;
        }
    }
    
    strcpy(filename,basename);
    strcat(filename,"-oddinterlace.tga");
    oddInterlace->writeFile(filename);
    
    strcpy(filename,basename);
    strcat(filename,"-eveninterlace.tga");
    evenInterlace->writeFile(filename);
    
    showSamplePoints(evenframe);
    strcpy(filename,basename);
    strcat(filename,"-evenAnnotated.tga");
    evenInterlace->writeFile(filename);
 
    return 0;
}*/

/* getpixelstreams converts two RGB color sequences into two pixelstreams.
 * Each RGB value is specified by a 32-bit binary number.
 * 
 * A pixelstream is a sequence of 2-bit binary numbers.
 * 
 * These two sequences come from the even and odd interlace lines of a 
 * captured analog television image.
 * 
 */
char computeColor(int pixelcolor) {
    //extract red, green, and blue components
    int red = (pixelcolor >> 16) & 0xFF;
    int grn = (pixelcolor >>  8) & 0xFF;
    int blu = (pixelcolor      ) & 0xFF;
    
    //compute colors assuming errors
    if(red>200 && grn>200 && blu>200)             return 1;   //white
    else if(red>grn && red>blu && red>50)         return 0;   //red
    else if(blu>grn && blu>red && blu>50)         return 2;   //blue
    else if(grn>red && grn>blu && grn>50)         return 3;   //green
    else                                          return 4;   //error
}

void getpixelstreams(int *rgb1, char *px1, int *rgb2, char *px2) {
    int ncells = 45 * 30;
    for(int i=0;i<ncells;i++) {
        px1[i] = computeColor(rgb1[i]);
        px2[i] = computeColor(rgb2[i]);
    }
}
        

void showSamplePoints(int *frame, int *sp1, int *sp2) {
    //Annotate the output. Put dots at encoding sample points.
    //Measurements taken from captureing test images and observing
    //color cell locations
    
    //sp1 and sp2 arrays are filled with 32-bit color values
    
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
            *(sp1+cell) = *(frame +z);
            *(frame + z)  = 0xFF000000;
            
            
            
           // printf("row %d, col %d: z1 = %x, z2 = %x\n",celrow,celcol,sp1[cell],sp2[cell]);
            cell++;
        }
    }
}
    
 void getstreams(int *frame, int *stream1, int *stream2) { 
    //Annotate the output. Put dots at encoding sample points.
    //Measurements taken from captureing test images and observing
    //color cell locations
    
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
                  
            int z = iy * width + ix;
            stream1[cell] = *(frame + z);
            //*(frame + z) = 0xFF00FFFF;
            
            z = (iy+1)*width +ix;
            stream2[cell] = *(frame + z);           
            //*(frame + z) = 0xFF000000;
            
            cell++;

        }
    }
}

