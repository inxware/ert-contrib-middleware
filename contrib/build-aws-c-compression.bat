echo "now do the aws-c-compression normal build"
rmdir /s /q aws-c-compression\build
copy aws-c-compression\CMakeLists.txt.original aws-c-compression\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-compression -B aws-c-compression\build -DLIBTYPE=SHARED -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-compression\build --target install

echo "now do the aws-c-compression dll build"
rmdir /s /q aws-c-compression\build
copy aws-c-compression\CMakeLists.txt.dll aws-c-compression\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-compression -B aws-c-compression\build -DLIBTYPE=SHARED -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-compression\build --target install
copy aws-c-compression\build\Debug\aws-c-compression.dll target_libs\lib\
