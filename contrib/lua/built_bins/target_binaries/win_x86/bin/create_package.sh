#Download Qt4.x.zip
#Make a directory Qt/
#unzip it
#run CMD
#set QMAKESPEC=win32-g++
#set PATH=D:\msys\mingw\bin;D:\msys\bin;%PATH%;


#Go to your Qt directory
#configure.exe -static -release -fast -no-exceptions -no-stl -no-sql-sqlite -no-qt3support  -platform win32-g++ -no-gif -no-libmng -qt-libjpeg -no-libtiff -no-dsp -no-vcproj  -no-rtti -no-qdbus -no-openssl -no-style-plastique -no-style-motif -no-style-cde -no-style-cleanlooks -no-webkit -mmx -sse -sse2 -no-script -no-multimedia -opensource -no-scripttools -no-opengl -3dnow

#Change the makespec file to put -O3
# patch Qt
# cd src; mingw32-make sub-src

#4/ Copy the needed files to make the layout like this :
#    (not yet automated)
#   include/
#       qt4/
#          QtCore/
#           QtGui/
#           src/
#               corelib/
#               gui/
#   lib/

VERSION=$1
echo "[+] Fixing include pathes"
# 1 / Fix the paths in the include files
for file in `find include/qt4/QtCore -type f -name '*h'` `find include/qt4/QtGui -type f -name '*h'`
do
    mv $file $file.tmp
    sed 's,..\/..\/src,..\/src,' $file.tmp > $file
    rm -f $file.tmp
done

# 2/ Remove the source files
echo "[+] Removing source files"
find include/qt4/src/corelib -name "*cpp"|xargs rm
find include/qt4/src/gui -name "*cpp"|xargs rm

# 3/ Create the PC files
echo "[+] Creating pkgconfig files"
mkdir -p lib/pkgconfig
rm -f lib/pkgconfig/QtCore.pc.in
cat > lib/pkgconfig/QtCore.pc.in << EOF
prefix=@@PREFIX@@
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include/qt4/QtCore

Name: Qtcore
Description: Qtcore Library
Version: $VERSION
Libs: -Wl,--subsystem,windows -mwindows -L\${libdir} -lQtCore -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lcomctl32 -ladvapi32 -lgdi32 -limm32 -lwsock32  
Cflags:  -I\${prefix}/include/qt4 -I\${includedir} -DQT_NODLL
EOF

rm -f lib/pkgconfig/QtGui.pc.in
cat > lib/pkgconfig/QtGui.pc.in << EOF
prefix=@@PREFIX@@
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include/qt4/QtGui

Name: Qtgui
Description: Qtgui Library
Version: $VERSION
Libs: -Wl,--subsystem,windows -mwindows -L\${libdir} -lqjpeg -lqtaccessiblewidgets -lQtGui -lQtCore  -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lcomctl32 -lcomdlg32 -ladvapi32 -lgdi32 -limm32 -lwsock32  
Cflags: -I\${prefix}/include/qt4 -I\${includedir} -DQT_NODLL
EOF

# 4/ Keep the script
echo "[+] Copying scripts"
mkdir -p bin
cp $0 bin/
#cp /usr/bin/moc-qt4 bin/moc
#cp /usr/bin/uic-qt4 bin/uic
#cp /usr/bin/rcc bin/rcc

# 5/ create packages
echo "[+] Creating tarball"
rm -rf qt4-$VERSION-win32-bin
mkdir qt4-$VERSION-win32-bin
cp -r include bin lib qt4-$VERSION-win32-bin
tar cj qt4-$VERSION-win32-bin > qt4-$VERSION-win32-bin.tar.bz2


