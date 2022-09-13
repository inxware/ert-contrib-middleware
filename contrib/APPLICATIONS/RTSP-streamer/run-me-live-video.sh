#README-SECTION
# For RTSP streaming on the ATOM/Blade Device use the inx-bonjout-rtsp-streamer-arm exectuable. The gst launch line should include an element with name=pay%d, which will then be streamed.
# For http Streaming the gst-auto-launch app is used with a time limited streaming if required using the start and end parameters  

#The following are used for development builds.
#test -f ./inx-bonjour-rtsp-streamer-x86 && LD_LIBRARY_PATH=${PWD}/lib GST_PLUGIN_PATH=${PWD}/lib/gstreamer-0.10 ./inx-bonjour-rtsp-streamer-x86 "filesrc location=testvid_baseline_128.mpeg4 ! qtdemux ! rtph264pay pt=96 name=pay0 "
#test -f ./inx-bonjour-rtsp-streamer-arm && LD_LIBRARY_PATH=${PWD}/lib-arm GST_PLUGIN_PATH=${PWD}/lib-arm/gstreamer-0.10 ./inx-bonjour-rtsp-streamer-arm \

# Include some test configs - First Just capture
if [ "${1}" == "test-capture" ];then
##Test it's alive:
 gst-auto-launch qtmux name=mux mux.src ! filesink location= ./H264ENC_FUNC_00041.mov omx_camera mode=1 focus=3 exposure=1 awb=1 name=cam cam.src \
 ! "video/x-raw-yuv-strided, format=(fourcc)NV12, width=480, height=320, framerate=24/1" ! tee name=t \
 t. ! queue name=q1 ! omx_h264enc input-buffers=2 output-buffers=2 bitrate=1000000 profile=1 level=128 ! queue name=q2 ! h264parse output-format=sample access-unit=true ! queue ! mux.video_0 \
 t. ! queue name=q3 ! v4l2sink device="/dev/video2" min-queued-bufs=2 sync=false crop-top=0 crop-left=0 crop-width=640 crop-height=480 0:play 10:eos
exit
fi

if [ "${1}" == "RTP-stream" ];then
# Something spewing data ...  Parameters
gst-auto-launch -m omx_camera mode=1 focus=3 exposure=1 awb=1 name=cam cam.src \
! "video/x-raw-yuv-strided, format=(fourcc)NV12, width=480, height=320, framerate=24/1" ! tee name=t t. \
! queue name=q1 ! omx_h264enc input-buffers=2 output-buffers=2 bitrate=1000000 profile=1 level=128 ! queue name=q2 \
! h264parse output-format=sample access-unit=true ! queue ! "video/x-h264,width=1280,height=720,framerate=24/1" ! rtph264pay  ! udpsink  host=192.168.3.3 port=8888 sync=false \
t. ! queue name=q3 ! v4l2sink device="/dev/video2" min-queued-bufs=2 sync=false crop-top=0 crop-left=0 crop-width=640 crop-height=480 0:play 120:eos
fi

if [ "${1}" == "tcpip-stream" ];then
gst-auto-launch -m  omx_camera mode=1 focus=3 exposure=1 awb=1 name=cam cam.src \
! "video/x-raw-yuv-strided, format=(fourcc)NV12, width=480, height=320, framerate=24/1" ! tee name=t t. \
! queue name=q1 ! omx_h264enc input-buffers=2 output-buffers=2 bitrate=1000000 profile=1 level=128 ! queue name=q2 \
! h264parse output-format=sample access-unit=true ! queue ! mpegtsmux name=mux \
t. ! queue name=q3 ! v4l2sink device="/dev/video2" min-queued-bufs=2 sync=false crop-top=0 crop-left=0 crop-width=640 crop-height=480 \
mux.src   !  tcpserversink port=8888   0:play 280:eos
exit
fi

if [ "${1}" == "rtsp-stream" ];then
./inx-bonjour-rtsp-streamer-arm "omx_camera mode=1 focus=3 exposure=1 awb=1 name=cam cam.src ! video/x-raw-yuv-strided, format=(fourcc)NV12, width=480, height=320, framerate=24/1 ! tee name=t t. ! queue name=q1 ! omx_h264enc input-buffers=2 output-buffers=2 bitrate=1000000 profile=1 level=128 ! queue name=q2 ! h264parse output-format=sample access-unit=true ! queue ! rtph264pay pt=96 name=pay0 t. ! queue name=q3 ! v4l2sink device=/dev/video2 min-queued-bufs=2 sync=false crop-top=0 crop-left=0 crop-width=640 crop-height=480 "
exit
fi


#mpegtsmux name=mux ! tcpserversink port=PORT

