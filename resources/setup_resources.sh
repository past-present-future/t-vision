#!/bin/bash 
#OUTPUT="$(v4l2-ctl --list-devices)"
MODULE="v4l2loopback"
CAMERA_NAME="natureYUV_steam_YUV-320x180"

if lsmod | grep -wq "$MODULE"; then
    echo "$MODULE is loaded!"
    echo "Checking for existing v4l2 devices"
    V4L2_DEVICE_LIST="$(v4l2-ctl --list-devices)"
    echo $V4L2_DEVICE_LIST
    if [[ $V4L2_DEVICE_LIST == *"$CAMERA_NAME"* ]];then
	echo "Virtual camera exists"
	V4L2_DEVICE_LIST="${V4L2_DEVICE_LIST##*$CAMERA_NAME}"
	V4L2_DEVICE_LIST="${V4L2_DEVICE_LIST##*dev}"
	V4L2_DEVICE_LIST="${V4L2_DEVICE_LIST##*video}"
	VIDEO_NR=$V4L2_DEVICE_LIST
	echo "Using /dev/video$VIDEO_NR, $CAMERA_NAME"
    else
	echo "Didn't detect any devices"
    fi

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
	echo "No video device. Binding video source to /dev/video0"
	VIDEO_NR=0
    fi

    sudo modprobe v4l2loopback devices=1 video_nr=$VIDEO_NR card_label="$CAMERA_NAME" exclusive_caps=1
fi


#OUTPUT="$(v4l2-ctl --list-devices | grep video?)"

#echo $OUTPUT
#if [[ "$OUTPUT" == *"$CAMERA_NAME"* ]]; then
    #echo "Camera already here"
#else
    #echo $(echo $OUTPUT | grep video\*)
    #sudo modprobe v4l2loopback devices=1 video_nr=0 card_label="$CAMERA_NAME" exclusive_caps=1
#fi

ffmpeg -stream_loop -1 -re -i nature_320x180.mp4 -f v4l2 -vcodec rawvideo -pix_fmt yuv420p /dev/video$VIDEO_NR 
#sudo modprobe -r v4l2loopback
#scrcpy --video-source=camera --v4l2-sink=/dev/video$VIDEO_NR --camera-size=640x360 --video-codec-options-KEY_COLOR_FORMAT:int=17 --no-audio --no-videoplay-back
