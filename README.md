# Pixel Stream Information Transfer (PXIT)

### Description
Pixel stream information transfer is a telecommunications technique for sending digital information using a stream of video images. 

The technique involves:

1. Converting a set of files into video images
2. Broadcasting the images as part of a television program
3. Receiving the images and recovering the set of files

### Table of Contents
1. pxit-encoder - converts files into a sequence of images
2. targa2video.sh - converts the images into a video file
3. Procedure for broadcasting and receiving video
4. pxit-decoder -converts received video into files
5. pxit-scope - a debugging tool to extract data from images created by pxit-encoder

### Installation
The software was built and tested on Arch Linux. To build the executables pxit-encode, pxit-decode, and pxit-scope
you should obtain [the software](https://github.com/pxitgit/pxit-tools.git), uncompress the zip file, switch to the directory containing the source code, and type 'make' on the command line.  Make will write the output programs into a subdirectory named bin.



