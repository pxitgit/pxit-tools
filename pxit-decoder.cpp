//pxit-decoder - software to convert a directory of images into a file.

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



int main(int argc, char *argv[]){

    //Validate inputs.  Expect only a path to the input file.
    if(argc != 2) {
        printf("Usage: %s <path to input>\n",argv[0]);
        return 0;
    }
    
    printf("\t**********Welcome to pxit-decoder**********\n\n");  
    
    //Validate input. Can we access the directory?
    DIR *dir;
    if ((dir = opendir (argv[1])) == NULL) {
        printf("Unable to open directory %s\n",argv[1]);
        return 0;
    }
 
    chdir(argv[1]);  //change working directory
    
    //Create object that converts images into a file
    ImageProcessor *processor = new ImageProcessor();

    //process all files with .tga extension
    struct dirent *entry;
    char buf[100];

    while ((entry = readdir (dir)) != NULL) {
        strcpy(buf,entry->d_name);
        char *ptr = buf + strlen(buf)-4;

        if(!strcmp(ptr, ".tga")) {
            TargaImage *tga = new TargaImage(buf,width,height);
            //extract image bitmap
            int *frame = (int *)tga->getFrame();
            
            /* ******************************************** */
            processor->processImage(frame);
            /* ******************************************** */
            
            delete(tga);
        }
    }
    closedir (dir);
    return 0;
}
 