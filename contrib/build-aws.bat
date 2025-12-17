echo "clean target_libs"
rmdir /s /q target_libs
mkdir target_libs

call build-aws-c-common.bat
call build-aws-c-cal.bat
call build-aws-c-io.bat
call build-aws-c-compression.bat
call build-aws-c-http.bat
call build-aws-c-mqtt.bat
call build-aws-lc.bat
