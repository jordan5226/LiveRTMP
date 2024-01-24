# LiveRTMP
A simple demo for pushing RTMP stream, using OpenCV + FFmpeg + Qt.  
The video/audio source can be from a webcam or RTSP stream.  
  
# Environment Configuration
***Visual Studio 2019***  
***Qt 6.7.0***  
  
***Visual Studio 2019 Qt Plug-in***  
1. qt-vsaddin-msvc2019-3.0.2.vsix  
[https://download.qt.io/official_releases/vsaddin/3.0.2/](https://download.qt.io/official_releases/vsaddin/3.0.2/)  

2. Setting Qt version in Visual Studio after installation:  
`Extensions -> Qt VS Tools -> Qt Versions`  
![Imgur Image](https://imgur.com/RyGjvsj.png)  
  
***OpenCV 4.7.0***  
Use CMake to configure and generate the build files.  
Note: Enable `WITH_FFMPEG` flag.  
[https://github.com/opencv/opencv/archive/4.7.0.zip](https://github.com/opencv/opencv/archive/4.7.0.zip)  
  
***FFmpeg 2023-09-30 win64-gpl-shared***  
Use release build from here:  
[https://github.com/BtbN/FFmpeg-Builds/releases](https://github.com/BtbN/FFmpeg-Builds/releases)  
  
  
# Usage
1. Source from local webcam  
`LiveRTMP.exe -camera -output rtmp://192.168.1.126/live`  
  
2. Source from RTSP stream  
`LiveRTMP.exe -src rtsp://192.168.1.134:8554/webcam.h264 -output rtmp://192.168.1.126/live`  
I use nginx-rtmp as the rtmp server.  
  