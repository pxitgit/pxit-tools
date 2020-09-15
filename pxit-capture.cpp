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

#define CLEAR(x) memset(&(x), 0, sizeof(x))

 
int flag = 0;
void showSamplePoints(int *frame);

void yuv2rgb(u_char *buf, int *frame);

void *handler(void *arg) {
    printf("handler called\n");
    char ans[10];
    while(1) {
        //sleep(1);
        scanf(" %s",ans);
        flag = 1;
        printf("tick\n");
 
    }
}


int main(int argc, char *argv[]) {
    
    //Validate inputs.  Expect only a path to the output directory.
    if(argc != 2) {
        printf("Usage: %s <path to output directory>\n",argv[0]);
        return 0;
    }

    //Validate input. Can we access the directory?
    DIR *dir;
    if ((dir = opendir (argv[1])) == NULL) {
        printf("Unable to open directory %s\n",argv[1]);
        return 0;
    }
 
    if(chdir(argv[1]) == -1) { //change working directory   
        printf("Unable to change directory to %s\n",argv[1]);
        perror("chdir");
        return 0;
    }

    //create a thread to handle user inputs
    pthread_t thread;


    if(pthread_create(&thread,NULL,handler,NULL)) {
        perror("pthread_create");
        return 0;
    }


    printf("\t******Welcome to pxit-capture*******\n\n");  
    
    int fd;  //file descriptor for video capture device
    struct v4l2_buffer buffer;                //info on buffer in device RAM
    struct v4l2_capability cap;             //capture device capabilities     
    struct v4l2_format format;              //specify video stream format
    struct v4l2_requestbuffers bufrequest;   //ask for device-based buffer
 


    struct ram {                            //Memory mapped RAM
        void *start;
        size_t length;
    } *buffers;

    TargaImage *tga = new TargaImage(720,480);  //useful for debugging

    ImageProcessor *processor = new ImageProcessor();  

    
    int *frame = (int *)tga->getFrame();  //use memory from TARGA object

    //Open the video capture device
    if((fd = open("/dev/video0", O_RDWR)) < 0){
        perror("open");
        printf("Check video device.\n");
        exit(1);
    }
    
    //Make sure the device can capture streaming video
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0){ //Get device capabilities
        perror("VIDIOC_QUERYCAP");
        exit(1);
    }
    
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        fprintf(stderr, "Device lacks capture capability.\n");
        exit(1);
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)){
        fprintf(stderr, "Device lacks streaming capabilite.\n");
        exit(1);
    }
    
    // *********************************************************
    struct v4l2_input input;
struct v4l2_standard standard;

memset(&input, 0, sizeof(input));

if (-1 == ioctl(fd, VIDIOC_G_INPUT, &input.index)) {
    perror("VIDIOC_G_INPUT");
    exit(EXIT_FAILURE);
}

if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
    perror("VIDIOC_ENUM_INPUT");
    exit(EXIT_FAILURE);
}

printf("Current input %s supports:\n", input.name);

memset(&standard, 0, sizeof(standard));
standard.index = 0;

while (0 == ioctl(fd, VIDIOC_ENUMSTD, &standard)) {
    if (standard.id & input.std)
        printf("%s\n", standard.name);

    standard.index++;
}


    
 printf("set standard to ntsc\n");   
 //set standard to NTSC
v4l2_std_id std_id;
std_id = V4L2_STD_NTSC;   
if (-1 == ioctl(fd, VIDIOC_S_STD, &std_id)) {
    perror("VIDIOC_S_STD");
    exit(EXIT_FAILURE);
}

memset(&standard, 0, sizeof(standard));
standard.index = 0;

while (0 == ioctl(fd, VIDIOC_ENUMSTD, &standard)) {
    if (standard.id & std_id) {
           printf("Current video standard: %s\\n", standard.name);
          break;
    }

    standard.index++;
}

//Display current video standard

printf("now set video format\n");
    //Set video format for the device. 
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0){
        perror("VIDIOC_S_FMT");
        exit(1);
    }
        // *********************************************************
    //Set the frame rate if the devlce supports it
    struct v4l2_streamparm streamparm;
    CLEAR(streamparm);

    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_G_PARM, &streamparm) < 0) {
        perror("VIDIOC_G_PARM");
        exit(1);
    }
    
    if(!(streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
        //Device allows frame rate setting
        auto &tpf = streamparm.parm.capture.timeperframe;
        //Set frame rate to 29.97 for NTSC TV
        tpf.numerator = 1001;
        tpf.denominator = 30000;
        ioctl(fd,VIDIOC_S_PARM,&streamparm);
    } else {
        printf("Device does not allow frame rate setting\n");
    }
    
    
   //Ask the driver to allocate buffers on device RAM
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 32;
    if(ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0){
        perror("VIDIOC_REQBUFS");
        exit(1);
    }
    
    int nbuffers = bufrequest.count;
    printf("Obtained %d buffers on capture device\n",nbuffers);
    
    //Get RAM for array of addresses mapped to device RAM   
    buffers = (struct ram *)calloc(nbuffers, sizeof(*buffers));
    
    //Initialize and enqueue each H/W buffer
    for(int i=0;i<nbuffers;i++) {
        //Fill in part of a V4L2 buffer structure
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        
        //Ask the driver to fill in the rest
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
        buffers[i].start = buffer_start;
        buffers[i].length = buffer.length;

        
        if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){
            perror("VIDIOC_QBUF");
            return 0;
        }  
    }
    
    //Activate streaming
    int type = buffer.type;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        perror("VIDIOC_STREAMON");
        return 0;
    }
    
    //Enter a loop that should run forever if there
    //are no errors.  Wait for a filled video buffer,
    //process it, and then return the buffer to the
    //V4L2 driver
 

    int cycle=0;
    while(1) {
        //Fill in a V4L2 buffer structure
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = cycle;     //use buffers in order

        //Get a filled image buffer from the V4L2 driver
        if(ioctl(fd, VIDIOC_DQBUF, &buffer) < 0){
            perror("VIDIOC_DQBUF");
            return -1;
        }
      
        //Note: 'frame' is a pointer to RAM within the a
        //      TargaImage object.  This makes it easy to
        //      take snapshots for diagnostic purposes.
        
        yuv2rgb((u_char *)buffers[cycle].start,frame);
        int rtn = processor->processImage(frame);
        
        
        if(rtn == -1) {
            printf("Checksum failed after first good one\n");
            showSamplePoints(frame);
            char filename[100];
            strcpy(filename, "/home/frank/pxit-tools/capturedFrame.tga");
            tga->writeFile(filename);
            
          
            //We should have 32 frames captured.
            for(int i=0;i<nbuffers;i++) {
                 //Convert format from YUV2 to RGB
                yuv2rgb((u_char *)buffers[i].start,frame);
                processor->processImage(frame);

                char filename[100];
                sprintf(filename, "/home/frank/pxit-tools/capturedFrame%2d.tga",i);
                printf("create targa file %s\n",filename);
                tga->writeFile(filename);
            }
            return 0;

        }
        


        
 /*        if(first) {
            showSamplePoints(frame);
            char filename[100];
            strcpy(filename, "/home/frank/pxit-tools/capturedFrame.tga");
            tga->writeFile(filename);
            first = false;
        }*/
        
        //Return buffer to driver
        //memset(buffers[cycle].start,0, buffers[cycle].length); //BUG BUG.  This shouldn't be zeroed.
        
        if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){  
            perror("VIDIOC_QBUF");
            return 0;
        } 

      
        cycle++;
        if(cycle >= nbuffers) cycle=0;  
        if(flag == 1) {
            //Produce diagnostics 
            showSamplePoints(frame);
            char filename[100];
            strcpy(filename, "/home/frank/pxit-tools/capturedFrame.tga");
            tga->writeFile(filename);
            flag = 0;
        }
    }
        

    
    
    
    //We should have 32 frames captured.
    for(int i=0;i<nbuffers;i++) {
         //Convert format from YUV2 to RGB

         
        yuv2rgb((u_char *)buffers[i].start,frame);
                processor->processImage(frame);

        char filename[100];
            sprintf(filename, "/home/frank/pxit-tools/capturedFrame%2d.tga",i);
            printf("create targa file %s\n",filename);
            tga->writeFile(filename);
    }
}

void yuv2rgb(u_char *buffer_start, int *frame) {
    int Y, U, V;            

    for(int row=0;row<height;row++) {
        for(int col=0;col<width;col++) {
            int z = row*width + col;                  //offset into linear array of pixels
            u_char  *yptr = (u_char *)buffer_start + 2 * z;     //pointer to luminance of pixel at (r,c).
            Y = *yptr;							      //Luminance value
            
            //compute u and v values
            if (z & 1) { //pixel number is odd
                U = *(yptr - 1);
                V = *(yptr + 1);
            } else {
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
            
            //Now copy these RGB components into frame
            int color = 0xFF;   color <<= 8; //alpha
            color |= R;         color <<=8;  //red
            color |= G;         color <<= 8; //greem
            color |= B;                      //blue
            
            *(frame + z) = color;
        }
    }
}

void showSamplePoints(int *frame) {
    
    //Annotate the output. Put dots at encoding sample points.
    //Measurements taken from captureing test images and observing
    //color cell locations
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
            *(frame + z) = 0xFF00FFFF;

        }
    }
}


 
