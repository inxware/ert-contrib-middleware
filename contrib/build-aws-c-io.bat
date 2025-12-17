echo "now do the aws-c-io normal build"
rmdir /s /q aws-c-io\build
copy aws-c-io\CMakeLists.txt.original aws-c-io\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-io -B aws-c-io\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-io\build --target install

echo "now do the aws-c-io dll build"
rmdir /s /q aws-c-io\build
copy aws-c-io\CMakeLists.txt.dll aws-c-io\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-io -B aws-c-io\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-io\build --target install
copy aws-c-io\build\Debug\aws-c-io.dll target_libs\lib\
