#!/bin/bash
#
# Populate the bin and library dirs just for this package
#|   |-- cslib 				
#|   |-- csdir

TARGET_TYPE=$1

########################## Start ####################################
TARGET_TREE_BASE=${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}
#TARGET_SOURCE_PREFIX="../target_lib_builds/$TARGET_TYPE" THIS WILL BE USED FOR MAKING per COMPOENTS BUILDS
TARGET_TREE_RUNTIME_SUPPORT="${TARGET_TREE_BASE}/target_packages"
TARGET_SOURCE_BASE="${TARGET_TREE_BASE}/${BUILD_INSTALL_DIR}"
EHSCORE_SUPPORT_LIB="corelib"
COMPONENT_SUPPORT_LIB="cslib"
COMPONENT_SUPPORT_DIR="csdir"
COMPONENT_SUPPORT_BIN="csbin"  

########################## Copy the runtime directory binaries ###########################################
echo Making directory $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB}
mkdir -p $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB} 
#rm -f $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB}/* # just remove the files leave svn tree as is!!!

echo Copying libraries libs
pushd ${TARGET_SOURCE_BASE}/lib
#find ./build -path "*.svn*" -prune -o -print |grep "\.so"
cp --parents `find ./ -path "*.svn*" -prune -o -print |grep "\.so"` $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB}
popd

#cp  -RP ${TARGET_SOURCE_BASE}/lib/* $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB}
#echo Removing unwanted files
#rm `find $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB} -name "*.a"`
#rm `find $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB} -name "*.la"`
#rm `find $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB} -name "*.o"`
#rm `find $TARGET_TREE_RUNTIME_SUPPORT/${COMPONENT_SUPPORT_LIB} -name "*.h"`
#echo Created the target structures for $TARGET_TYPE - Finished
echo XXXXXXXXXXXX

