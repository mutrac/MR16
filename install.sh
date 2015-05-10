#!/bin/sh
# install.sh - Installation script for the MR16
sudo apt-get update
sudo apt-get upgrade

# Basics
sudo apt-get install python-dev build-essential python-pip unzip -y -qq

## CMQ
sudo apt-get install arduino arduino-mk -y -qq
sudo apt-get install python-serial python-zmq -y -qq

## OBD
sudo apt-get install python-cherrypy3 -y -qq
sudo apt-get install mongodb -y -qq
sudo pip install pymongo

## HUD
sudo apt-get install python-tk -y -qq

# Install AV Codecs from APT
echo "Installing AV Codecs"
sudo apt-get install yasm -y -qq
sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libdc1394-22-dev libxine-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libv4l-dev -y -qq
sudo apt-get install libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libtheora-dev libvorbis-dev libx264-dev libxvidcore-dev  -y -qq
sudo apt-get install libtiff4-dev libjpeg-dev libjasper-dev -y -qq

# Install FFMPEG from source
echo "Installing ffmpeg"
cd /root/MR16/src
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
cd /root/MR16/src
sudo apt-get install cmake libgtk2.0-dev pkg-config -y -qq
unzip opencv-2.4.9.zip
cd opencv-2.4.9
mkdir release
cd release
cmake -D CV_BUILD_TYPE=RELEASE ..
make -j4
sudo make install

# Configure Boot
read -p "Are you sure you want to start automatically on boot [y/n]?" ans
if [ $ans = y -o $ans = Y -o $ans = yes -o $ans = Yes -o $ans = YES ]
    then
        echo "Removing LightDM ..."
        update-rc.d -f lightdm remove
        
        echo "Disabling Mouse (Unclutter) ..."
        apt-get install unclutter
        cp configs/unclutter /etc/default/
        
        echo "Installing to Boot Path ..."
        cp configs/rc.local /etc/
        chmod +x /etc/rc.local
        
        echo "Updating Custom Grub ..."
        cp configs/grub /etc/default
        update-grub
        
        echo "Reconfiguring Network Interfaces ..."
        cp configs/interfaces /etc/network
fi
if [ $ans = n -o $ans = N -o $ans = no -o $ans = No -o $ans = NO ]
    then
        echo "Aborting..."
fi

# Copy Arduino Libraries
echo "Copying libraries for arduino"
cd /root/MR16
cp -R lib/* /usr/share/arduino/libraries

# Clean up
echo "Cleaning up files/folders"
cd /root/MR16/src
rm -rf ffmpeg-0.11.1
rm -rf opencv-2.4.9
