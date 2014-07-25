## FFMPEG
sudo apt-get install yasm
sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libdc1394-22-dev libxine-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libv4l-dev
sudo apt-get install libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libtheora-dev libvorbis-dev libx264-dev libxvidcore-dev
sudo apt-get install libtiff4-dev libjpeg-dev libjasper-dev
wget http://ffmpeg.org/releases/ffmpeg-0.11.1.tar.bz2
tar -xvf ffmpeg-0.11.1.tar.bz2
cd ffmpeg-0.11.1/
./configure --enable-gpl --enable-libmp3lame --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libtheora --enable-libvorbis --enable-libx264 --enable-libxvid --enable-nonfree --enable-postproc --enable-version3 --enable-x11grab
make
sudo make install

# Fix for: 'videodev.h' not found
cd /usr/include/linux
sudo ln -s ../libv4l1-videodev.h videodev.h
sudo ln -s ../libavformat/avformat.h avformat.h

# OpenCV
wget http://ffmpeg.org/releases/ffmpeg-0.11.1.tar.bz2
unzip opencv-2.4.9.zip
cd opencv-2.4.9
mkdir release
cd release
cmake -D CV_BUILD_TYPE=RELEASE ..
make -j4
sudo make install
