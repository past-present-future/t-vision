#!/bin/bash 
#OUTPUT="$(v4l2-ctl --list-devices)"
MODULE="v4l2loopback"
CAMERA_NAME="natureYUV_steam_YUV-320x180"

if lsmod | grep -wq "$MODULE"; then
    echo "$MODULE is loaded!"
else
    echo "$MODULE is not loaded!\n Loading module...\n"
    OUTPUT="$(ls /dev/ | grep video)"
    if [[ $OUTPUT != "" ]];then
       echo $OUTPUT
       IFS="o" read loc ind <<< "$OUTPUT"
       echo "Index got: $ind"
       echo "Test got: $loc"
       VIDEO_NR=$(eval $ind + 1)
    else
	echo "No video device setting binding to /dev/video0"
	VIDEO_NR=0
    fi
    echo "No video device setting binding to /dev/video0"
    sudo modprobe v4l2loopback devices=1 video_nr=0 card_label="$CAMERA_NAME" exclusive_caps=1
fi


#OUTPUT="$(v4l2-ctl --list-devices | grep video?)"

#echo $OUTPUT
#if [[ "$OUTPUT" == *"$CAMERA_NAME"* ]]; then
    #echo "Camera already here"
#else
    #echo $(echo $OUTPUT | grep video\*)
    #sudo modprobe v4l2loopback devices=1 video_nr=0 card_label="$CAMERA_NAME" exclusive_caps=1
#fi

ffmpeg -stream_loop -1 -re -i nature_320x180.mp4 -f v4l2 -vcodec rawvideo -pix_fmt yuv420p /dev/video0 
sudo modprobe -r v4l2loopback

