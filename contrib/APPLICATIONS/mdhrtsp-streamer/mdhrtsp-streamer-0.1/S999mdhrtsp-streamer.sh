if [ "${1}" == "start" ]; then
	#start the syslink daemon if it is not running
	if [ -z `pidof syslink_daemon.out` ]; then
		/usr/bin/syslink_daemon.out /lib/firmware/omap4/base_image_sys_m3.xem3 /lib/firmware/omap4/base_image_app_m3.xem3
	fi
	mdhrtsp-streamer "" 8554 "DEVICEID" "raymarine-mfd-rtsp-path=RAYMARINEMFD|raymarine-mfd-model=MODELID|raymarine-mfd-serial=SERIAL" "v4l2src device=/dev/video3 always-copy=false queue-size=10 decimate=3 ! video/x-raw-yuv-strided, format=(fourcc)NV12, width=800, height=480, framerate=20/1 ! queue ! omx_h264enc input-buffers=2 output-buffers=2 bitrate=400000 profile=1 level=2048 ! queue ! h264parse output-format=sample access-unit=true ! queue ! rtph264pay pt=96 name=pay0" &
fi

if [ "${1}" == "stop" ]; then
	killall mdhrtsp-streamer
fi




