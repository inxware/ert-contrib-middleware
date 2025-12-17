echo "now do aws-c-cal normal build"
rmdir /s /q aws-c-cal\build
copy aws-c-cal\CMakeLists.txt.original aws-c-cal\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-cal -B aws-c-cal\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-cal\build --target install

echo "now do the aws-c-cal dll build"
rmdir /s /q aws-c-cal\build
copy aws-c-cal\CMakeLists.txt.dll aws-c-cal\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-cal -B aws-c-cal\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-cal\build --target install
copy aws-c-cal\build\Debug\aws-c-cal.dll target_libs\lib\
