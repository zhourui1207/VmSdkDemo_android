#!/bin/sh
rm -rf vmsdk
mkdir vmsdk
cd vmsdk
mkdir jniLibs
cd ..
cd app/build/intermediates/classes/release
jar -cvf vmsdk.jar com/jxll/vmsdk
mv vmsdk.jar ../../../../../vmsdk/
cd ../..
cp -rf jniLibs/release/* ../../../vmsdk/jniLibs/
cp -rf cmake/release/obj/* ../../../vmsdk/jniLibs/
