echo "do a normal build of aws-c-common to get the .lib file"
rmdir /s /q aws-c-common\build
copy aws-c-common\CMakeLists.txt.original aws-c-common\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-common -B aws-c-common\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-common\build --target install

echo "now do the aws-c-common dll build"
rmdir /s /q aws-c-common\build
copy aws-c-common\CMakeLists.txt.dll aws-c-common\CMakeLists.txt
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-c-common -B aws-c-common\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-c-common\build --target install
copy aws-c-common\build\Debug\aws-c-common.dll target_libs\lib\
