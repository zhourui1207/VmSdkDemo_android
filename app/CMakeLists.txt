# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.
# 定义宏
option(_ANDROID         "Set to switch to build use SSE"  ON)
if (_ANDROID)
    add_definitions(-D_ANDROID) # 宏定义 _ANDROID
    message(STATUS "Use _ANDROID")
else()
    message(STATUS "Use Nothing")
endif()
add_definitions(-DPHONE_PLATEFORM)
add_definitions(-D_ONLY_DECRYPT)
#add_definitions(-D__ANDROID__)

#定义一下，不然会报错 boost::system::throws 重定义
#add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)

# 相当于g++ 中的 -I参数
include_directories(/usr/local/Cellar/boost/1.61.0_1/include/)
include_directories(src/main/cpp/ffmpeg/)
include_directories(src/main/cpp/pb/)
include_directories(/Users/zhourui/Library/Android/sdk/ndk-bundle/sources/android/support/include/)

# 它相当于g++命令的-L选项的作用，也相当于环境变量中增加LD_LIBRARY_PATH的路径的作用
# link_directories("")

add_library(VmNet

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            # Associated headers in the same location as their source
            # file are automatically included.
            src/main/cpp/VmNet-lib.cpp
            src/main/cpp/plateform/VmNet.cpp
            src/main/cpp/plateform/MainModule.cpp
            src/main/cpp/plateform/StreamSessionManager.cpp
            src/main/cpp/opensdk/RtpUdpServer.cpp
            src/main/cpp/opensdk/StreamHeartbeatServer.cpp
            src/main/cpp/rtp/RtpPacket.cpp
            src/main/cpp/stream/StreamClient.cpp
            src/main/cpp/cipher/jw_cipher.c
            src/main/cpp/cipher/base64encode.c
            src/main/cpp/cipher/md5.c
            src/main/cpp/cipher/jw_aes.c
            src/main/cpp/uas/UasClient.cpp
            src/main/cpp/util/AsioTcpClient.cpp
            src/main/cpp/util/AsioUdpServer.cpp
            src/main/cpp/util/ThreadPool.cpp
            src/main/cpp/util/TaskThread.cpp
            src/main/cpp/util/Tasks.cpp
            src/main/cpp/util/Timer.cpp
            src/main/cpp/dp/HB.pb.cc
            src/main/cpp/rtsp/RtspTcpClient.cpp
            src/main/cpp/rtsp/md5.cpp
            src/main/cpp/rtsp/authTools.cpp
            )


add_library(VmPlayer

            SHARED

            src/main/cpp/VmPlayer-lib.cpp
            src/main/cpp/plateform/VmPlayer.cpp
            src/main/cpp/encdec/AACEncoder.cpp
            src/main/cpp/encdec/Decoder.cpp
            src/main/cpp/render/EGLRender.cpp
            src/main/cpp/cipher/jw_cipher.c
            src/main/cpp/cipher/base64encode.c
            src/main/cpp/cipher/md5.c
            src/main/cpp/cipher/jw_aes.c
            )

add_library(JWSmartConfig

            SHARED

            src/main/cpp/JWSmartConfig-lib.cpp
            src/main/cpp/smartconfig/WifiConfig.cpp
            src/main/cpp/smartconfig/smart_config.cpp
            src/main/cpp/util/AsioUdpServer.cpp
            )

add_library(JWDeviceDiscovery

            SHARED

            src/main/cpp/JWDeviceDiscovery-lib.cpp
            src/main/cpp/devicediscovery/device_discovery.cpp
            src/main/cpp/devicediscovery/DeviceDiscoveryManager.cpp
            src/main/cpp/util/AsioUdpServer.cpp
            src/main/cpp/util/HandlerThread.cpp
            src/main/cpp/util/MessageQueue.cpp
            src/main/cpp/util/Message.cpp
            src/main/cpp/util/Looper.cpp
            src/main/cpp/util/Handler.cpp
            )

add_library(ZRudt

            SHARED

            src/main/cpp/ZRudt-lib.cpp
            src/main/cpp/udt/UDTMultiplexer.cpp
            src/main/cpp/udt/UDTSocket.cpp
            src/main/cpp/udt/SendGlideWindow.cpp
            src/main/cpp/udt/SendLossList.cpp
            src/main/cpp/udt/ReceiveGlideWindow.cpp
            src/main/cpp/udt/ReceiveLossList.cpp
            src/main/cpp/udt/AckHistoryWindow.cpp
            src/main/cpp/util/AsioUdpServer.cpp
            src/main/cpp/util/ThreadPool.cpp
            src/main/cpp/util/TaskThread.cpp
            src/main/cpp/util/Tasks.cpp
            src/main/cpp/util/HandlerThread.cpp
            src/main/cpp/util/MessageQueue.cpp
            src/main/cpp/util/Message.cpp
            src/main/cpp/util/Looper.cpp
            src/main/cpp/util/Handler.cpp
            )


# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       VmNet

                       # Links the target library to the log library
                       # included in the NDK.
                       # 使用android_support.a库
                       /Users/zhourui/Library/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libandroid_support.a
                       "/Users/zhourui/Documents/c++workspace/DreamEngine/VmSdkDemo_android/app/src/main/jniLibs/${ANDROID_ABI}/libprotobuf-lite.so"
                       ${log-lib} )

target_link_libraries(VmPlayer
                      "/Users/zhourui/Documents/c++workspace/DreamEngine/VmSdkDemo_android/app/src/main/jniLibs/${ANDROID_ABI}/libJWEncdec.so"
                      EGL
                      GLESv2
                      ${log-lib}
                      android)

target_link_libraries(JWSmartConfig
                   /Users/zhourui/Library/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libandroid_support.a
                  ${log-lib})

target_link_libraries(JWDeviceDiscovery
                 /Users/zhourui/Library/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libandroid_support.a
                ${log-lib})

target_link_libraries(ZRudt
                    ${log-lib})

#link_libraries()
