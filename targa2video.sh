#!/bin/bash
#Convert directory of TARGA images into a video.
cle
nlen=2;

usage() {
	echo "Usage: $0 -i <input file> -o <output file> [-l <field length>]";	
	exit -1;
}
echo "$0 - convert Targa images to mp4 video";

while getopts i:o:l: OPTION
do
case "${OPTION}" 
in
  i) input="${OPTARG}";;
  o) output="${OPTARG}";;	  
  l) nlen="${OPTARG}";;

esac
done

if [ -z "$input" ]
then
	echo "No Input";
	usage
fi

if [ -z "$output" ]
then
	echo "No Output";
	usage
fi  

if [ $nlen -lt 1 ]
then
	echo "Positive length needed";
	usage
fi
  	  	
pattern="$input-%0$nlen";
pattern+="d.tga";

cmd="ffmpeg -r 30 -f image2 -s 720x480 -i $pattern -crf 25 -pix_fmt yuv420p ";
cmd+=$output;
#echo $cmd;
eval $cmd;



