if [ -n "${1}" ]
then
TARGET=$1
echo Cleaning $TARGET
rm -Rf ../target_libs/$TARGET
rm -Rf ../contrib/*/target_cs_packages/$TARGET/*
rm -Rf ../contrib/*/target_lib_builds/$TARGET/*
else
echo You must supply a target name
fi




