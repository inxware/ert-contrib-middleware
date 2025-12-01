#makes both targets. These packages are for testing on basic hosts instead of installing
PREFIX="Raymarine-test-streamer-`date +%G-%m-%d`"
X86PREFIX=${PREFIX}_x86
ARMPREFIX=${PREFIX}_arm
#rm -rf ${ARMPREFIX}
rm -rf ${X86PREFIX}
#mkdir ${ARMPREFIX}
mkdir ${X86PREFIX}
#cp -Pr ./lib-arm ${ARMPREFIX} && cp ./inx-bonjour-rtsp-streamer-arm ${ARMPREFIX}/ && cp ./testvid.h264 ./${ARMPREFIX}/ && cp ./README ./${ARMPREFIX}/ && cp ./run-me-test-video.sh ./${ARMPREFIX}
cp -Pr ./lib ${X86PREFIX} && cp  ./rtsp-streaming-mp4.sh ${X86PREFIX}/ && cp ./README ./${X86PREFIX}/ && cp ./run-me-test-video-mp4.sh ./${X86PREFIX}
pushd .
cd ./${X86PREFIX}
rm -rf `find -name ".svn"`
popd 
test -f inx-bonjour-rtsp-streamer-x86 && tar -cjf ${X86PREFIX}.tar.bz2  ${X86PREFIX}
#test -f inx-bonjour-rtsp-streamer-arm  && tar -cjf ${ARMPREFIX}.tar.bz2 ${ARMPREFIX}

