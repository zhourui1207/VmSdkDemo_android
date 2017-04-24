#!/bin/sh
rm -rf vmsdk
mkdir vmsdk
cd vmsdk
mkdir jniLibs
cd ..
cd app/build/intermediates/classes/release
jar -cvf JWVmsdk.jar com/joyware/vmsdk
mv JWVmsdk.jar ../../../../../vmsdk/
cd ../..
cp -rf jniLibs/release/* ../../../vmsdk/jniLibs/
cp -rf cmake/release/obj/* ../../../vmsdk/jniLibs/
