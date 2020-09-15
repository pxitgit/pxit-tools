//TargeImage.cpp - utilities to read and write TARGA files

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
#include <stdlib.h>
#include <string.h>
//#include <windows.h>

#include "TargaImage.h"

long *TargaImage::getFrame() {return frame;}

TargaImage::TargaImage(int Width, int Height) { //ctor for writing images

	tga = NULL; 
	frame = new long[Width*Height];
	memset(frame,0,4*Width*Height);

	header.width  = _width  = Width;
	header.height = _height = Height; 
    
    overwriteOK = true;  //disable check for existing file.

}//end ctor

TargaImage::TargaImage(char *Filename, int Width, int Height) {//ctor (reading)

	frame = new long[Width*Height];
	memset(frame,0,4*Width*Height);

	 //Store image geometry in class variables
	 _width  = Width;  
	 _height = Height;

	//Store filename (may be useful for diagnostic messages)
	strcpy(filename,Filename);

	//Open the file containing the image
	tga = fopen(filename,"rb");
	if(!tga) {
		printf("TargaImage: error opening %s\n",filename);
		perror("ERROR");
		exit(0);
	} else
		readheader(tga); //Read and validate the header

    decode(tga);
}//end ctor

void  TargaImage::decode(FILE *tga) {

	if(header.img_type == 10) {  //Header indicates Run Length Encoded (RLE)
		
		//int bpp;
		//if(header.bpp == 24)	bpp = 3;
		//else					bpp = 4;

		unsigned char rle_hdr;
		for(int y = 0; y < _height; y++) {
			int x = 0;

			while(x < _width) {
				fread(&rle_hdr,1,1,tga);
				int RLE = rle_hdr & 0x80;
				int packetLength = (rle_hdr & 0x7F) + 1;
				long color;
				int j;
				int nBytes = 3;
				if(header.bpp == 32) nBytes = 3;

				if(RLE) { //Process an RLE packet 
					fread(&color,nBytes,1,tga);
					color |= 0xFF000000;
					for(j=0; j<packetLength; j++)
					   *(frame + y * _width + x++) = color;

				} else { //Process a raw packet

					for(j=0; j<packetLength; j++) {
					   fread(&color,nBytes,1,tga);
					   *(frame + y*_width + x++) = color;	      
					}
				}
			}//while   
		}//end for

	} else if(header.bpp == 32) { //non-encoded, 32-bit image
		fread(frame,4,_width*_height,tga);

	} else if(header.bpp == 24) { //non-encoded, 24-bit image.Convert to 32
		long *tmp = new long[_width*_height];
		memset(tmp,0,3*_width*_height);
		fread(tmp,3,_width*_height,tga);
		char *sptr = (char *)tmp;
		char *fptr = (char *)frame;

		for(int i=0;i<_width*_height;i++) {
			*fptr++ = *sptr++;
			*fptr++ = *sptr++;
			*fptr++ = *sptr++;
			*fptr++ = (char)0xFF;
		}
		delete [] tmp;
	}

	//Do we need to invert the image?
	if(header.misc != 0x20) {  //Yes, invert.
		long color;
		//Now flip the image
		for(int row = 0; row < _height/2; row ++)
			for(int col = 0;col < _width; col ++) {
			   color = *(frame + _width*row + col);
			   *(frame + _width*row + col) = 
                                *(frame + _width*(_height-1-row) +col);
			   *(frame + _width*(_height-1-row) + col) = color;
			}
	}

}//end decode

void TargaImage::fillBox(int top,int bottom,int left,int right,long color){
  int row, col;
  for(row = top;row < bottom; row++)
    for(col=left; col<right; col++)
      *(frame + header.width*row + col) = color;
}

void TargaImage::writeFile(char *name, int bpp) {

	FILE *tga=NULL;


  tga = fopen(name,"wb");
  if(!tga) {
    printf("TargaImage: File Open Error\n");
    exit(0);
  }
  writeheader(tga, bpp);
  if(header.bpp == 32)
    fwrite(frame,4,_width*_height,tga);
  else {
    long *lp = frame;
    for(int i=0; i< _width * _height; i++)
      fwrite(lp++,1,3,tga);
    
  }
  fclose(tga);
}

void TargaImage::writeheader(FILE *tga, int bpp) {
  header.id_len = 0;
  header.map_type = 0;
  header.img_type = 2;  //Run length encoded True Color.
  header.map_first = 0;
  header.map_len = 0;
  header.map_entry_size = 0;
  header.x = 0;
  header.y = 0;
  header.bpp = bpp;
  header.misc = 0x20;
 
  fputc(header.id_len,tga);
  fputc(header.map_type,tga);
  fputc(header.img_type,tga);
  fputc(header.map_first % 256, tga);
  fputc(header.map_first / 256, tga);
  fputc(header.map_len % 256, tga);
  fputc(header.map_len / 256, tga);
  fputc(header.map_entry_size, tga);
  fputc(header.x % 256, tga);
  fputc(header.x / 256, tga);
  fputc(header.y % 256, tga);
  fputc(header.y / 256, tga);
  fputc(header.width % 256, tga);
  fputc(header.width / 256, tga);
  fputc(header.height % 256, tga);
  fputc(header.height / 256, tga);
  fputc(header.bpp, tga);
  fputc(header.misc, tga);
}//end writeHeader

void TargaImage::readheader(FILE *tga) {

  int hi, lo;
  header.id_len   = (char)fgetc(tga);
  header.map_type = (char)fgetc(tga);
  header.img_type = (char)fgetc(tga);

  lo = fgetc(tga); hi = fgetc(tga); header.map_first = (hi<<8)|lo;
  lo = fgetc(tga); hi = fgetc(tga); header.map_len   = (hi<<8)|lo;

  header.map_entry_size = fgetc(tga);
  lo = fgetc(tga); hi = fgetc(tga); header.x         = (hi<<8)|lo;
  lo = fgetc(tga); hi = fgetc(tga); header.y         = (hi<<8)|lo;
  lo = fgetc(tga); hi = fgetc(tga); header.width     = (hi<<8)|lo;
  lo = fgetc(tga); hi = fgetc(tga); header.height    = (hi<<8)|lo;

  header.bpp  = fgetc(tga);
  header.misc = fgetc(tga);

  //Now validate the header
  int ok = 1;
  //Only handle files with no ID, no color palette, and origin at (0,0).
  if(header.id_len || header.x || header.y)        ok = 0;
  if(header.map_type != 0 && header.map_type != 1) ok = 0;

  //Check image type. We only handle uncompressed True Color images. 
  if(header.img_type != 2 && header.img_type != 10)     ok = 0;
  if(header.bpp != 32 && header.bpp != 24)              ok = 0; 
  if(!ok) {
    printf("TargaImage: Can't handle %s\n",filename); 
    printf("id_len = %d\n",header.id_len);
    printf("map_type = %d\n",header.map_type);
    printf("img_type = %d\n",header.img_type);
    printf("map_first = %d\n",header.map_first);
    printf("map_len = %d\n",header.map_len);
    printf("map_entry_size = %d\n",header.map_entry_size);
    printf("x = %d, y = %d\n",header.x, header.y);
    printf("width = %d, height = %d\n",header.width, header.height);
    printf("bpp = %d, misc = %x\n",header.bpp, header.misc);
    exit(0);
  }

//debug 
displayHeader();
}//end readHeader

void TargaImage::displayHeader() {
    printf("id_len = %d\n",header.id_len);
    printf("map_type = %d\n",header.map_type);
    printf("img_type = %d\n",header.img_type);
    printf("map_first = %d\n",header.map_first);
    printf("map_len = %d\n",header.map_len);
    printf("map_entry_size = %d\n",header.map_entry_size);
    printf("x = %d, y = %d\n",header.x, header.y);
    printf("width = %d, height = %d\n",header.width, header.height);
    printf("bpp = %d, misc = %x\n",header.bpp, header.misc);
}

/*
int TargaImage::makeColor(int red, int green, int blue){
  int color = 0xFF000000 | (red<<16) | (green<<8) | blue;
  return color;
}
*/












