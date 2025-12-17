echo "now do the aws-c-mqtt normal build"
rmdir /s /q aws-c-mqtt\build
copy aws-c-mqtt\CMakeLists.txt.original aws-c-mqtt\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-mqtt -B aws-c-mqtt/build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-mqtt/build --target install

echo "now do the aws-c-mqtt dll build"
rmdir /s /q aws-c-mqtt\build
copy aws-c-mqtt\CMakeLists.txt.dll aws-c-mqtt\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-mqtt -B aws-c-mqtt/build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-mqtt/build --target install
copy aws-c-mqtt\build\Debug\aws-c-mqtt.dll target_libs\lib\


