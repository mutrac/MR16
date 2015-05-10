#!/bin/sh
# install.sh - Installation script for the MR16
sudo apt-get update
sudo apt-get upgrade

# Python Base
sudo apt-get install python-dev build-essential python-pip

# Arduino
sudo apt-get install arduino arduino-mk -y -qq

# MongoDB
sudo apt-get install mongodb -y -qq
sudo pip install pymongo

# CMQ
sudo apt-get install zmq python-serial python-zmq -y -qq

# OBD
sudo apt-get install python-cherrypy3 -y -qq

# HUD
sudo apt-get install python-tkinter -y -qq

# Install AV Codecs from APT
echo "Installing AV Codecs"
sudo apt-get install yasm
sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libdc1394-22-dev libxine-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libv4l-dev -y -qq
sudo apt-get install libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libtheora-dev libvorbis-dev libx264-dev libxvidcore-dev  -y -qq
sudo apt-get install libtiff4-dev libjpeg-dev libjasper-dev -y -qq

# Install FFMPEG from source
echo "Installing ffmpeg"
tar -xvf ffmpeg-0.11.1.tar.bz2
cd ffmpeg-0.11.1/
./configure --enable-gpl --enable-libmp3lame --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libtheora --enable-libvorbis --enable-libx264 --enable-libxvid --enable-nonfree --enable-postproc --enable-version3 --enable-x11grab
make
sudo make install

# Fix for: 'videodev.h' not found
echo "Fix for 'videodev.h not found'"
cd /usr/include/linux
sudo ln -s ../libv4l1-videodev.h videodev.h
sudo ln -s ../libavformat/avformat.h avformat.h

# Install OpenCV from source
echo "Installing OpenCV 2.4.9"
sudo apt-get install cmake libgtk2.0-dev pkg-config
unzip opencv-2.4.9.zip
cd opencv-2.4.9
mkdir release
cd release
cmake -D CV_BUILD_TYPE=RELEASE ..
make -j4
sudo make install

echo "Removing LightDM"
update-rc.d lightdm remove

echo "Cleaning up files"
cd ../..
rm -rf ffmpeg-0.11.1
rm ffmpeg-0.11.1.tar.bz2
rm opencv-2.4.9.zip
rm -rf opencv-2.4.9
