#!/bin/bash 
sudo modprobe v4l2loopback devices=1 video_nr=0 card_label='big_buck_bunny_steam_YUV-320x180' exclusive_caps=1
v4l2-ctl --list-devices
ffmpeg -stream_loop -1 -re -i BigBuckBunny_320x180.mp4 -f v4l2 -vcodec rawvideo -pix_fmt yuv420p /dev/video0 
