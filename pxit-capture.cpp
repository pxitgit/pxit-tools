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

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/mman.h>
#include <libv4l2.h>
#include <libv4lconvert.h>
#include <dirent.h>
#include <pthread.h>
#include "ImageProcessor.h"
#include "TargaImage.h"
#include <errno.h>

//Macros
#define CLEAR(x) memset(&(x), 0, sizeof(x))

//Constants
const int verbose = 0;  //produce verbose output

//Function prototypes
int  initializeDevice(int *nBuffers);        //Returns file descriptor to capture device. The number
                                            //of frame buffers available in the hardware RAM is returned.
                                            
void showSamplePoints(int *frame);          //Create an image indicating sample points
int  validateInputs(int argc, char **argv); //Make sure we have a valid directory to write into
void yuv2rgb(u_char *buf, int *frame);      //Convert from YUV to RGB color spaces.

int validateInputs(int argc, char **argv) {
    if(argc != 2) {
        printf("Usage: %s <path to output directory>\n",argv[0]);
        return 0;
    } 

   //Can we access the directory?
    DIR *dir;
    if ((dir = opendir (argv[1])) == NULL) {
        printf("Unable to open directory %s\n",argv[1]);
        return 0;
    }
    
    //Can we change the working directory?
    if(chdir(argv[1]) == -1) { //change working directory   
        printf("Unable to change directory to %s\n",argv[1]);
        perror("chdir");
        return 0;
    }

    //Everything looks good.  Return success.
    return 1;    
}

int initializeDevice(int *nbuffers) {
    struct v4l2_capability cap;             //capture device capabilities 
    struct v4l2_format format;              //specify video stream format
    struct v4l2_requestbuffers bufrequest;   //ask for device-based buffer
    v4l2_std_id std_id;
        
    //Open the video capture device
    int fd;
    if((fd = open("/dev/video0", O_RDWR)) < 0){
        perror("open");
        printf("Check video device.\n");
        return -1;
    }
    
    //Make sure the device can capture streaming video
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0){ //Get device capabilities
        perror("VIDIOC_QUERYCAP");
        return -1;
    }
    
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        fprintf(stderr, "Device lacks capture capability.\n");
        return -1;
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)){
        fprintf(stderr, "Device lacks streaming capabilite.\n");
        return -1;
    }
    
    //If verbose is set, list all the formats supported by the 
    //current input.
    if(verbose) {
        struct v4l2_input input;
        struct v4l2_standard standard;
        memset(&input, 0, sizeof(input));

        if (-1 == ioctl(fd, VIDIOC_G_INPUT, &input.index)) {
            perror("VIDIOC_G_INPUT");
        }

        if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
            perror("VIDIOC_ENUM_INPUT");
        }

        printf("Current input is %s. It supports the following standards:\n", input.name);

        memset(&standard, 0, sizeof(standard));
        standard.index = 0;

        while (0 == ioctl(fd, VIDIOC_ENUMSTD, &standard)) {
            if (standard.id & input.std) printf("%s\n", standard.name);
            standard.index++;
        }
    }
    
    //Set video standard to NTSC
    std_id = V4L2_STD_NTSC;   
    if (-1 == ioctl(fd, VIDIOC_S_STD, &std_id)) {
        perror("VIDIOC_S_STD");
        return -1;
    }
    
    //Set video format for the device. 
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0){
        perror("VIDIOC_S_FMT");
        return -1;
    }
    
    //Ask the driver to allocate buffers on device RAM
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 32;      //#max on our device
    if(ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0){
        perror("VIDIOC_REQBUFS");
        return -1;
    }
    
    //This is what we actually got.
    *nbuffers = bufrequest.count;
    
    return fd;
}

int main(int argc, char *argv[]) {
    
    //Validate input directory and change to it if possible.
    if(!validateInputs(argc, argv)) return -1;
    
    //Initalize capture device.  Returns file descriptor to capture device.
    int nbuffers;
    int fd = initializeDevice(&nbuffers);
    if(fd <=0) return -1;  //initializeDevice failed


    printf("\t******Welcome to pxit-capture*******\n\n");  
    printf("Using %d RAM buffers on capture device\n",nbuffers);
    printf("Writing received files to %s\n",argv[1]);
    

  


   //Note: 'frame' is a pointer to RAM within the a
    //      TargaImage object.  This makes it easy to
    //      take snapshots for diagnostic purposes.
    TargaImage *tga = new TargaImage(720,480);  //useful for debugging
    int *frame = (int *)tga->getFrame();  //use memory from TARGA object
    
    //This analyzes captured images.
    ImageProcessor *processor = new ImageProcessor();  

    //These are memory-mapped addresses of the device-resident RAM
    struct ram_t {                            
        void *start;
        size_t length;
    } *memoryMapInfo; 
    
    memoryMapInfo = (struct ram_t *)calloc(nbuffers, sizeof(ram_t));
    
    //Initialize and enqueue each H/W buffer
    struct v4l2_buffer buffer;
    for(int i=0;i<nbuffers;i++) {
        
        //Fill in part of a V4L2 buffer structure and ask the
        //driver to fill in the rest.
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        
        if(ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0){
            perror("VIDIOC_QUERYBUF");
            exit(1);
        }
        
        //Use the information returned to create a memory
        //map between a userspace addresses and device RAM
        void* buffer_start = mmap(
            NULL,
            buffer.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            buffer.m.offset //from start of device RAM
        );
        
        if(buffer_start == MAP_FAILED){
            perror("mmap");
            return 0;
        }
        
        //Now save the starting userspace address of buffer
        memoryMapInfo[i].start = buffer_start;
        memoryMapInfo[i].length = buffer.length;
        
        if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){
            perror("VIDIOC_QBUF");
            return 0;
        }  
    }
    
    //Activate streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        perror("VIDIOC_STREAMON");
        return 0;
    }
    

    /* *****************************************************/
    /*              ENTER PROCESSING LOOP                  */
    /* *****************************************************/
    int cycle=0;
    while(1) {
        
        //Dequeue a buffer filled by the V4L driver
        if(ioctl(fd, VIDIOC_DQBUF, &buffer) < 0){
            perror("VIDIOC_DQBUF");
            return -1;
        }
      
        //Do a color space transformation on the device-resident
        //image and write the RGB version to 'frame'.
        yuv2rgb((u_char *)memoryMapInfo[cycle].start,frame);
        
        //Let the image processor examine the frame.
        int rtn = processor->processImage(frame);
        
        if(rtn == -1) { 
            if(verbose) {
                printf("Checksum failed after first good one\n");
                showSamplePoints(frame);
                char filename[100];
                strcpy(filename,argv[1]);
                strcat(filename,"-failedFrame.tga");
                tga->writeFile(filename);
                printf("Dumped failing image to %s\n",filename);
                return 0;  //We have to stop.  Depending on the vide source
                           //there may be nothing but bad checksums from here on.
            }
        }
        
        //Return the buffer to the driver.
        if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){  
            perror("VIDIOC_QBUF");
            return 0;
        } 

        //Cycle through the buffers
        if(++cycle >= nbuffers) cycle=0;  
    }
}

/* 
 * yuv22rgb converts an image from YUV to RGB color spaces. A pointer to the
 * image in YUV format is input.  The program returns the converted image in 'rgb'.
 */
 
void yuv2rgb(u_char *yuv, int *rgb) {
    int Y, U, V;            

    //Loop over every pixel in the image (though we don't have to)
    for(int row=0;row<height;row++) {
        for(int col=0;col<width;col++) {
            
            int z = row*width + col;                     //pixel number
            
            u_char  *yptr = (u_char *)yuv + (2 * z);     //pointer to luminance 
            
            Y = *yptr;							               //Luminance value
            
            //Compute u and v values for every pixel in the image
            if (z & 1) { 
                U = *(yptr - 1);    //U and V values surround the luminance value for odd numbered pixels.
                V = *(yptr + 1);
            } else {                //U and V values follow the luminance value for even numbered pixels.
                U = *(yptr + 1);
                V = *(yptr + 3);
            }

            int C = Y - 16;
            int D = U - 128;
            int E = V - 128;

            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;

            //Enforce bounds on color intensity
            if (R > 255) R = 255; else if (R < 0) R = 0;
            if (G > 255) G = 255; else if (G < 0) G = 0;
            if (B > 255) B = 255; else if (B < 0) B = 0;
            
            //Now copy these RGB components into rgb
            int color = 0xFF;   color <<= 8; //alpha (transparency)
            color |= R;         color <<= 8; //red
            color |= G;         color <<= 8; //greem
            color |= B;                      //blue
            
            *(rgb + z) = color;
        }
    }
}

void showSamplePoints(int *frame) {
    
    //Annotate the output. Put dots at encoding sample points.
    
    //Loop over the 45x30 array of color cells
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
            *(frame + z) = 0xFF000000;
        }
    }
}