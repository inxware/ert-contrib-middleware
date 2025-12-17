echo "First install nasm from https://www.nasm.us/ and add it to your path"
set PATH=%PATH%;C:\Users\Patrick\Documents\work\inx\nasm-2.16.01
echo "You will also need go installing from https://go.dev/doc/install"
echo "You will also need perl installing from https://strawberryperl.com/"
echo "now do aws-lc normal build"
rmdir /s /q aws-lc\build
cmake -G "Visual Studio 16 2019" -A Win32 -S aws-lc -B aws-lc\build -DCMAKE_INSTALL_PREFIX=C:\Users\Patrick\Documents\work\inx\target_libs -DCMAKE_PREFIX_PATH=C:\Users\Patrick\Documents\work\inx\target_libs
cmake --build aws-lc\build --target install
