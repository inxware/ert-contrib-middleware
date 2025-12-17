echo "now do the aws-c-http normal build"
rmdir /s /q aws-c-http\build
copy aws-c-http\CMakeLists.txt.original aws-c-http\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-http -B aws-c-http/build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-http/build --target install

echo "now do the aws-c-http dll build"
rmdir /s /q aws-c-http\build
copy aws-c-http\CMakeLists.txt.dll aws-c-http\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-http -B aws-c-http/build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-http/build --target install
copy aws-c-http\build\Debug\aws-c-http.dll target_libs\lib\


