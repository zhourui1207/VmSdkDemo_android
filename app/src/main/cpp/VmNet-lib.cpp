#include <android/log.h>
#include <atomic>
#include <jni.h>
#include <string>

#include "plateform/VmNet.h"
#include "plateform/ErrorCode.h"

#define TAG "VmNet-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__) // 定义LOGF类型

const char *CLASS_PATH_VMNET = "com/jxll/vmsdk/VmNet";

const char *METHOD_NAME_ON_SERVER_CONNECT_STATUS = "onServerConnectStatus";
const char *METHOD_SIG_ON_SERVER_CONNECT_STATUS = "(Z)V";

const char *METHOD_NAME_ON_STREAM_CONNECT_STATUS = "onStreamConnectStatus";
const char *METHOD_SIG_ON_STREAM_CONNECT_STATUS = "(IZLjava/lang/Object;)V";

const char *METHOD_NAME_ON_STREAM = "onStream";
const char *METHOD_SIG_ON_STREAM = "(III[BIIIZLjava/lang/Object;)V";

const char *METHOD_NAME_ON_REAL_ALARM = "onRealAlarm";
const char *METHOD_SIG_ON_REAL_ALARM = "(Ljava/lang/String;IIII)V";

const char *METHOD_NAME_LIST_ADDDITEM = "addItem";
const char *METHOD_SIG_DEPTREES = "(ILjava/lang/String;III)V";
const char *METHOD_SIG_CHANNELS = "(ILjava/lang/String;IILjava/lang/String;IIII)V";
const char *METHOD_SIG_RECORDS = "(IILjava/lang/String;Ljava/lang/String;)V";
const char *METHOD_SIG_ALARMS = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;ILjava/lang/String;ILjava/lang/String;Ljava/lang/String;ILjava/lang/String;)V";

const char *METHOD_NAME_PLAYADDRESSHOLDER_INIT = "init";
const char *METHOD_SIG_PLAYADDRESSHOLDER_INIT = "(ILjava/lang/String;ILjava/lang/String;I)V";

const char *METHOD_NAME_TALKADDRESSHOLDER_INIT = "init";
const char *METHOD_SIG_TALKADDRESSHOLDER_INIT = "(ILjava/lang/String;I)V";

const char *METHOD_NAME_STREAMIDHOLDER_SET = "setStreamId";
const char *METHOD_SIG_STREAMIDHOLDER_SET = "(I)V";

enum VMNET_INIT_STATUS {
    NO_INIT = 0, INITING = 1, INITED = 2, UNINITING = 3
};

std::atomic<VMNET_INIT_STATUS> g_init(NO_INIT);  // 使用原子变量，使多线程安全
JavaVM *g_pJavaVM = nullptr;
jclass g_vmNet = nullptr;
jmethodID g_onServerConnectStatus = nullptr;
jmethodID g_onStreamConnectStatus = nullptr;
jmethodID g_onStream = nullptr;
jmethodID g_onRealAlarm = nullptr;


// 回调函数-------------------------------------------------------------------------
void onServerConnectStatus(bool bIsConnected) {
  if (g_pJavaVM && g_onServerConnectStatus) {
    JNIEnv *jniEnv = nullptr;
    if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) !=
        JNI_OK) {  // 两种方法都可以
//    if (g_pJavaVM->AttachCurrentThread(&jniEnv, nullptr) != JNI_OK) {
      LOGE("onServerConnectStatus 获取jniEnv失败！");
      return;
    }
    if (jniEnv) {
      jniEnv->CallStaticVoidMethod(g_vmNet, g_onServerConnectStatus, bIsConnected);
    }
  }
}

void onStreamConnectStatus(unsigned uStreamId, bool bIsConnected, void *pUser) {
//  LOGE("开始调用onStreamConnectStatus函数");
  if (g_pJavaVM && g_onStreamConnectStatus) {
    JNIEnv *jniEnv = nullptr;
    if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) !=
        JNI_OK) {  // 两种方法都可以
      LOGE("onStreamConnectStatus 获取jniEnv失败！");
      return;
    }
    if (jniEnv) {
      jniEnv->CallStaticVoidMethod(g_vmNet, g_onStreamConnectStatus, uStreamId, bIsConnected,
                                   static_cast<jobject>(pUser));
    }
  }
}

// 码流回调
void onStream(unsigned uStreamId, unsigned uStreamType,
              char payloadType, char *pBuffer, int nLen, unsigned uTimeStamp,
              unsigned short seqNumber, bool isMark, void *pUser) {
//  LOGE("开始调用onStream函数");
  if (g_pJavaVM && g_onStream) {

    JNIEnv *jniEnv = nullptr;
    if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) !=
        JNI_OK) {  // 两种方法都可以
      LOGE("onStream 获取jniEnv失败！");
      return;
    }
    // 由于java中的char使用unicode编码，为16位双字节，所以应该将c++中的char转变成byte给java层使用
    jbyteArray buffer = jniEnv->NewByteArray(nLen);
    jniEnv->SetByteArrayRegion(buffer, 0, nLen, (jbyte *) pBuffer);

    jniEnv->CallStaticVoidMethod(g_vmNet, g_onStream, uStreamId, uStreamType, payloadType, buffer,
                                 nLen, uTimeStamp, seqNumber, isMark, static_cast<jobject>(pUser));
    jniEnv->DeleteLocalRef(buffer);
  }
}

void onRealAlarm(const char *sFdId, int nChannel, unsigned nAlarmType, unsigned nParam1,
                 unsigned nParam2) {
  if (g_pJavaVM && g_onRealAlarm) {
    JNIEnv *jniEnv = nullptr;
    if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) !=
        JNI_OK) {  // 两种方法都可以
      LOGE("g_onRealAlarm 获取jniEnv失败！");
      return;
    }
    if (jniEnv) {
      jstring fdId = jniEnv->NewStringUTF(sFdId);
      jniEnv->CallStaticVoidMethod(g_vmNet, g_onRealAlarm, fdId, nChannel, nAlarmType, nParam1,
                                   nParam2);
      jniEnv->DeleteLocalRef(fdId);
    }
  }
}



// 接口函数-------------------------------------------------------------------------
extern "C" {

jboolean Java_com_jxll_vmsdk_VmNet_Init(JNIEnv *env, jobject /* this */,
                                              jint maxThreadCount) {
  // 如果已经被初始化，那么应该先执行unInit释放资源后再重新init
  if (g_init.load() == INITED) {
    return false;
  }

  LOGI("Java_com_jxll_vmsdk_VmNet_Init(%d)", maxThreadCount);
  g_init.store(INITING);

  // 保存java虚拟机对象
  env->GetJavaVM(&g_pJavaVM);

  // 获取VmNet sdk通信接口类
  jclass vmNet = env->FindClass(CLASS_PATH_VMNET);
  // 本地引用无法共享，必须要保存成全局引用
  g_vmNet = reinterpret_cast<jclass>(env->NewGlobalRef(vmNet));
  env->DeleteLocalRef(vmNet);

  if (g_vmNet == nullptr) {
    LOGE("找不到[%s]类!", CLASS_PATH_VMNET);
    g_init.store(NO_INIT);
    return false;
  }

  // 获取服务器状态回调函数
  g_onServerConnectStatus = env->GetStaticMethodID(g_vmNet, METHOD_NAME_ON_SERVER_CONNECT_STATUS,
                                                   METHOD_SIG_ON_SERVER_CONNECT_STATUS);
  if (g_onServerConnectStatus == nullptr) {
    env->DeleteGlobalRef(g_vmNet);
    LOGE("找不到[%s]方法!", METHOD_NAME_ON_SERVER_CONNECT_STATUS);
    g_init.store(NO_INIT);
    return false;
  }

  // 获取码流状态回调函数
  g_onStreamConnectStatus = env->GetStaticMethodID(g_vmNet, METHOD_NAME_ON_STREAM_CONNECT_STATUS,
                                                   METHOD_SIG_ON_STREAM_CONNECT_STATUS);
  if (g_onStreamConnectStatus == nullptr) {
    env->DeleteGlobalRef(g_vmNet);
    LOGE("找不到[%s]方法!", METHOD_NAME_ON_STREAM_CONNECT_STATUS);
    g_init.store(NO_INIT);
    return false;
  }

  // 获取码流回调函数
  g_onStream = env->GetStaticMethodID(g_vmNet, METHOD_NAME_ON_STREAM, METHOD_SIG_ON_STREAM);
  if (g_onStream == nullptr) {
    env->DeleteGlobalRef(g_vmNet);
    LOGE("找不到[%s]方法!", METHOD_NAME_ON_STREAM);
    g_init.store(NO_INIT);
    return false;
  }

  // 获取实时报警回调函数
  g_onRealAlarm = env->GetStaticMethodID(g_vmNet, METHOD_NAME_ON_REAL_ALARM,
                                         METHOD_SIG_ON_REAL_ALARM);
  if (g_onRealAlarm == nullptr) {
    env->DeleteGlobalRef(g_vmNet);
    LOGE("找不到[%s]方法!", METHOD_NAME_ON_REAL_ALARM);
    g_init.store(NO_INIT);
    return false;
  }

  bool bInit = VmNet_Init(maxThreadCount);
  if (bInit) {
    g_init.store(INITED);
  } else {
    g_init.store(NO_INIT);
  }
  return bInit;
}

void Java_com_jxll_vmsdk_VmNet_UnInit(JNIEnv *env, jobject) {
  if (g_init.load() == INITED && g_pJavaVM) {
    LOGI("Java_com_jxll_vmsdk_VmNet_UnInit()");

    g_init.store(UNINITING);

//    g_pJavaVM = nullptr;  // 这里不可以把javaVM置为空，因为线程中需要用到该指针解绑android线程
    VmNet_UnInit();
    env->DeleteGlobalRef(g_vmNet);
    g_init.store(NO_INIT);
  }
}

jint Java_com_jxll_vmsdk_VmNet_Connect(JNIEnv *env, jobject, jstring serverAddr,
                                             jint serverPort) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  const char *sServerAddr = nullptr;
  sServerAddr = env->GetStringUTFChars(serverAddr, 0);
  LOGI("Java_com_jxll_vmsdk_VmNet_Connect(%s, %d)", sServerAddr, serverPort);

  return VmNet_Connect(sServerAddr, serverPort, onServerConnectStatus);
}

void Java_com_jxll_vmsdk_VmNet_Disconnect(JNIEnv *env, jobject) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_Disconnect()");

  VmNet_Disconnect();
}

jint Java_com_jxll_vmsdk_VmNet_Login(JNIEnv *env, jobject, jstring loginName,
                                           jstring loginPwd) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  const char *sLoginName = nullptr;
  sLoginName = env->GetStringUTFChars(loginName, 0);
  const char *sLoginPwd = nullptr;
  sLoginPwd = env->GetStringUTFChars(loginPwd, 0);
  LOGI("Java_com_jxll_vmsdk_VmNet_Login(%s, ******)", sLoginName);

  return VmNet_Login(sLoginName, sLoginPwd);
}

void Java_com_jxll_vmsdk_VmNet_Logout(JNIEnv *env, jobject) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_Logout()");

  VmNet_Logout();
}

jint Java_com_jxll_vmsdk_VmNet_GetDepTrees(JNIEnv *env, jobject, jint pageNo, jint pageSize,
                                                 jobject depTreesHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  if (pageSize <= 0) {
    return ERR_CODE_PARAM_ILLEGAL;
  }

  LOGI("Java_com_jxll_vmsdk_VmNet_GetDepTrees(%d, %d)", pageNo, pageSize);

  TVmDepTree depTrees[pageSize];
  unsigned uSize = 0;
  int ret = VmNet_GetDepTrees(pageNo, pageSize, depTrees, uSize);

  if (ret == ERR_CODE_OK && uSize > 0) {
    jclass depTreesCls = env->GetObjectClass(depTreesHolder);

    jmethodID addItem = env->GetMethodID(depTreesCls, METHOD_NAME_LIST_ADDDITEM,
                                         METHOD_SIG_DEPTREES);

    env->DeleteLocalRef(depTreesCls);

    if (addItem == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_LIST_ADDDITEM);
      return ret;
    }

    for (unsigned i = 0; i < uSize; ++i) {
      jstring depName = env->NewStringUTF(depTrees[i].sDepName);

      env->CallVoidMethod(depTreesHolder, addItem, depTrees[i].nDepId, depName,
                          depTrees[i].nParentId, depTrees[i].uOnlineChannelCounts,
                          depTrees[i].uOfflineChannelCounts);

      env->DeleteLocalRef(depName);
    }

  }

  return ret;
}

jint Java_com_jxll_vmsdk_VmNet_GetChannels(JNIEnv *env, jobject, jint pageNo, jint pageSize,
                                                 jint depId, jobject channelsHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  if (pageSize <= 0) {
    return ERR_CODE_PARAM_ILLEGAL;
  }

  LOGI("Java_com_jxll_vmsdk_VmNet_GetChannels(%d, %d, %d)", pageNo, pageSize, depId);

  TVmChannel channels[pageSize];
  unsigned uSize = 0;
  int ret = VmNet_GetChannels(pageNo, pageSize, depId, channels, uSize);

  if (ret == ERR_CODE_OK && uSize > 0) {
    jclass channelsCls = env->GetObjectClass(channelsHolder);

    jmethodID addItem = env->GetMethodID(channelsCls, METHOD_NAME_LIST_ADDDITEM,
                                         METHOD_SIG_CHANNELS);
    env->DeleteLocalRef(channelsCls);

    if (addItem == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_LIST_ADDDITEM);
      return ret;
    }

    for (unsigned i = 0; i < uSize; ++i) {
      jstring fdId = env->NewStringUTF(channels[i].sFdId);
      jstring channelName = env->NewStringUTF(channels[i].sChannelName);

      env->CallVoidMethod(channelsHolder, addItem, channels[i].nDepId, fdId,
                          channels[i].nChannelId, channels[i].uChannelType, channelName,
                          channels[i].uIsOnLine, channels[i].uVideoState, channels[i].uChannelType,
                          channels[i].uRecordState);

      env->DeleteLocalRef(fdId);
      env->DeleteLocalRef(channelName);
    }
  }

  return ret;
}

jint Java_com_jxll_vmsdk_VmNet_GetRecords(JNIEnv *env, jobject, jint pageNo, jint pageSize,
                                                jstring fdId, jint channelId, jint beginTime,
                                                jint endTime, jboolean isCenter,
                                                jobject recordsHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  if (pageSize <= 0) {
    return ERR_CODE_PARAM_ILLEGAL;
  }

  const char *sFdId = nullptr;
  sFdId = env->GetStringUTFChars(fdId, 0);

  LOGI("Java_com_jxll_vmsdk_VmNet_GetRecords(%d, %d, %s, %d, %d, %d, %d)", pageNo, pageSize,
       sFdId, channelId, beginTime, endTime, isCenter);

  TVmRecord records[pageSize];
  unsigned uSize = 0;
  int ret = VmNet_GetRecords(pageNo, pageSize, sFdId, channelId, beginTime, endTime, isCenter,
                             records, uSize);

  if (ret == ERR_CODE_OK && uSize > 0) {
    jclass recordsCls = env->GetObjectClass(recordsHolder);

    jmethodID addItem = env->GetMethodID(recordsCls, METHOD_NAME_LIST_ADDDITEM,
                                         METHOD_SIG_RECORDS);

    env->DeleteLocalRef(recordsCls);

    if (addItem == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_LIST_ADDDITEM);
      return ret;
    }

    for (unsigned i = 0; i < uSize; ++i) {
      jstring playbackUrl = env->NewStringUTF(records[i].sPlaybackUrl);
      jstring downloadUrl = env->NewStringUTF(records[i].sDownloadUrl);

      env->CallVoidMethod(recordsHolder, addItem, records[i].uBeginTime, records[i].uEndTime,
                          playbackUrl, downloadUrl);

      env->DeleteLocalRef(playbackUrl);
      env->DeleteLocalRef(downloadUrl);
    }

  }

  return ret;
}

jint Java_com_jxll_vmsdk_VmNet_GetAlarms(JNIEnv *env, jobject, jint pageNo, jint pageSize,
                                               jstring fdId, jint channelId, jint channelBigType,
                                               jstring beginTime, jstring endTime,
                                               jobject alarmsHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  if (pageSize <= 0) {
    return ERR_CODE_PARAM_ILLEGAL;
  }

  const char *sFdId = nullptr;
  sFdId = env->GetStringUTFChars(fdId, 0);
  const char *sBeginTime = nullptr;
  sBeginTime = env->GetStringUTFChars(beginTime, 0);
  const char *sEndTime = nullptr;
  sEndTime = env->GetStringUTFChars(endTime, 0);

  LOGI("Java_com_jxll_vmsdk_VmNet_GetAlarms(%d, %d, %s, %d, %d, %s, %s)", pageNo, pageSize,
       sFdId, channelId, channelBigType, sBeginTime, sEndTime);

  TVmAlarm alarms[pageSize];
  unsigned uSize = 0;
  int ret = VmNet_GetAlarms(pageNo, pageSize, sFdId, channelId, channelBigType, sBeginTime,
                            sEndTime, alarms, uSize);

  if (ret == ERR_CODE_OK && uSize > 0) {
    jclass alarmsCls = env->GetObjectClass(alarmsHolder);

    jmethodID addItem = env->GetMethodID(alarmsCls, METHOD_NAME_LIST_ADDDITEM,
                                         METHOD_SIG_ALARMS);
    env->DeleteLocalRef(alarmsCls);
    if (addItem == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_LIST_ADDDITEM);
      return ret;
    }

    for (unsigned i = 0; i < uSize; ++i) {
      jstring alarmId = env->NewStringUTF(alarms[i].sAlarmId);
      jstring lFdId = env->NewStringUTF(alarms[i].sFdId);
      jstring fdName = env->NewStringUTF(alarms[i].sFdName);
      jstring channelName = env->NewStringUTF(alarms[i].sChannelName);
      jstring alarmTime = env->NewStringUTF(alarms[i].sAlarmTime);
      jstring alarmName = env->NewStringUTF(alarms[i].sAlarmName);
      jstring alarmSubName = env->NewStringUTF(alarms[i].sAlarmSubName);
      jstring photoUrl = env->NewStringUTF(alarms[i].sPhotoUrl);

      env->CallVoidMethod(alarmsHolder, addItem, alarmId, lFdId, fdName, alarms[i].nChannelId,
                          channelName, alarms[i].uChannelBigType, alarmTime, alarms[i].uAlarmCode,
                          alarmName, alarmSubName, alarms[i].uAlarmType, photoUrl);

      env->DeleteLocalRef(alarmId);
      env->DeleteLocalRef(lFdId);
      env->DeleteLocalRef(fdName);
      env->DeleteLocalRef(channelName);
      env->DeleteLocalRef(alarmTime);
      env->DeleteLocalRef(alarmName);
      env->DeleteLocalRef(alarmSubName);
      env->DeleteLocalRef(photoUrl);
    }

  }

  return ret;
}

void Java_com_jxll_vmsdk_VmNet_StartReceiveRealAlarm(JNIEnv *env, jobject) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }

  LOGI("Java_com_jxll_vmsdk_VmNet_StartReceiveRealAlarm()");

  VmNet_StartReceiveRealAlarm(onRealAlarm);
}

void Java_com_jxll_vmsdk_VmNet_StopReceiveRealAlarm(JNIEnv *env, jobject) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_StopReceiveRealAlarm()");

  VmNet_StopReceiveRealAlarm();
}

jint Java_com_jxll_vmsdk_VmNet_OpenRealplayStream(JNIEnv *env, jobject, jstring fdId,
                                                        jint channelId, jboolean isSub,
                                                        jobject playAddressHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  const char *sFdId = nullptr;
  sFdId = env->GetStringUTFChars(fdId, 0);
  unsigned uMonitorId, uVideoPort, uAudioPort = 0;
  char sVideoAddr[100];
  char sAudioAddr[100];

  LOGI("Java_com_jxll_vmsdk_VmNet_OpenRealplayStream(%s, %d, %d)", sFdId, channelId, isSub);
  int ret = VmNet_OpenRealplayStream(sFdId, channelId, isSub, uMonitorId, sVideoAddr, uVideoPort,
                                     sAudioAddr, uAudioPort);

  if (ret == ERR_CODE_OK && playAddressHolder) {
    jclass playAddressCls = env->GetObjectClass(playAddressHolder);

    jmethodID init = env->GetMethodID(playAddressCls, METHOD_NAME_PLAYADDRESSHOLDER_INIT,
                                      METHOD_SIG_PLAYADDRESSHOLDER_INIT);
    if (init == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_PLAYADDRESSHOLDER_INIT);
      env->DeleteLocalRef(playAddressCls);
      return ret;
    }

    jstring videoAddr = env->NewStringUTF(sVideoAddr);
    jstring audioAddr = env->NewStringUTF(sAudioAddr);

    env->CallVoidMethod(playAddressHolder, init, uMonitorId, videoAddr, uVideoPort, audioAddr,
                        uAudioPort);

    env->DeleteLocalRef(playAddressCls);
    env->DeleteLocalRef(videoAddr);
    env->DeleteLocalRef(audioAddr);
  }
  return ret;
}

void Java_com_jxll_vmsdk_VmNet_CloseRealplayStream(JNIEnv *env, jobject, jint monitorId) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_CloseRealplayStream(%d)", monitorId);

  VmNet_CloseRealplayStream(monitorId);
}

jint Java_com_jxll_vmsdk_VmNet_StartTalk(JNIEnv *env, jobject, jstring fdId,
                                                        jint channelId, jobject talkAddressHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  const char *sFdId = nullptr;
  sFdId = env->GetStringUTFChars(fdId, 0);
  unsigned uTalkId, uTalkPort = 0;
  char sTalkAddr[100];

  LOGI("Java_com_jxll_vmsdk_VmNet_StartTalk(%s, %d)", sFdId, channelId);
  int ret = 0;
//  int ret = VmNet_StartTalk(sFdId, channelId, uTalkId, sTalkAddr, uTalkPort);
//
//  if (ret == ERR_CODE_OK && talkAddressHolder) {
//    jclass talkAddressCls = env->GetObjectClass(talkAddressHolder);
//
//    jmethodID init = env->GetMethodID(talkAddressCls, METHOD_NAME_TALKADDRESSHOLDER_INIT,
//                                      METHOD_SIG_TALKADDRESSHOLDER_INIT);
//    if (init == nullptr) {
//      LOGE("找不到[%s]方法!", METHOD_NAME_TALKADDRESSHOLDER_INIT);
//      env->DeleteLocalRef(talkAddressCls);
//      return ret;
//    }
//
//    jstring talkAddr = env->NewStringUTF(sTalkAddr);
//
//    env->CallVoidMethod(talkAddressHolder, init, uTalkId, talkAddr, uTalkPort);
//
//    env->DeleteLocalRef(talkAddressCls);
//    env->DeleteLocalRef(talkAddr);
//  }
  return ret;
}

void Java_com_jxll_vmsdk_VmNet_StopTalk(JNIEnv *env, jobject, jint talkId) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_StopTalk(%d)", talkId);

//  VmNet_StopTalk(talkId);
}

jint Java_com_jxll_vmsdk_VmNet_OpenPlaybackStream(JNIEnv *env, jobject, jstring fdId,
                                                        jint channelId, jboolean isCenter,
                                                        jint beginTime, jint endTime,
                                                        jobject playAddressHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  const char *sFdId = nullptr;
  sFdId = env->GetStringUTFChars(fdId, 0);
  unsigned uMonitorId, uVideoPort, uAudioPort = 0;
  char sVideoAddr[100];
  char sAudioAddr[100];

  LOGI("Java_com_jxll_vmsdk_VmNet_OpenPlaybackStream(%s, %d, %d, %d, %d)", sFdId, channelId,
       isCenter, beginTime, endTime);
  int ret = VmNet_OpenPlaybackStream(sFdId, channelId, isCenter, beginTime, endTime, uMonitorId,
                                     sVideoAddr, uVideoPort, sAudioAddr, uAudioPort);

  if (ret == ERR_CODE_OK && playAddressHolder) {
    jclass playAddressCls = env->GetObjectClass(playAddressHolder);

    jmethodID init = env->GetMethodID(playAddressCls, METHOD_NAME_PLAYADDRESSHOLDER_INIT,
                                      METHOD_SIG_PLAYADDRESSHOLDER_INIT);
    if (init == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_PLAYADDRESSHOLDER_INIT);
      env->DeleteLocalRef(playAddressCls);
      return ret;
    }

    jstring videoAddr = env->NewStringUTF(sVideoAddr);
    jstring audioAddr = env->NewStringUTF(sAudioAddr);

    env->CallVoidMethod(playAddressHolder, init, uMonitorId, videoAddr, uVideoPort, audioAddr,
                        uAudioPort);

    env->DeleteLocalRef(playAddressCls);
    env->DeleteLocalRef(videoAddr);
    env->DeleteLocalRef(audioAddr);
  }
  return ret;
}

void Java_com_jxll_vmsdk_VmNet_ClosePlaybackStream(JNIEnv *env, jobject, jint monitorId) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_ClosePlaybackStream(%d)", monitorId);

  VmNet_ClosePlaybackStream(monitorId);
}

jint Java_com_jxll_vmsdk_VmNet_ControlPlayback(JNIEnv *env, jobject, jint monitorId, jint controlId,
                                                     jstring action, jstring param) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  const char *sAction = nullptr;
  sAction = env->GetStringUTFChars(action, 0);
  const char *sParam = nullptr;
  sParam = env->GetStringUTFChars(param, 0);

  LOGI("Java_com_jxll_vmsdk_VmNet_ControlPlayback(%d, %d, %s, %s)", monitorId, controlId, sAction, sParam);

  return VmNet_ControlPlayback(monitorId, controlId, sAction, sParam);
}

jint Java_com_jxll_vmsdk_VmNet_StartStream(JNIEnv *env, jobject, jstring address, jint port,
                                                 jobject cb, jobject streamIdHolder) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }

  const char *sAddress = nullptr;
  sAddress = env->GetStringUTFChars(address, 0);

  // 这里一定要传递GlobalRef给码流线程，如果直接传cb的话，由于是局部变量，后面回调时会失败，记得释放时删除
  jobject gCb = env->NewGlobalRef(cb);

  LOGI("Java_com_jxll_vmsdk_VmNet_StartStream(%s, %d, %zd)", sAddress, port, gCb);
  unsigned uStreamId = 0;
  int ret = VmNet_StartStream(sAddress, port, onStream, onStreamConnectStatus, gCb, uStreamId);

  if (ret == ERR_CODE_OK) {
    jclass streamIdCls = env->GetObjectClass(streamIdHolder);

    jmethodID set = env->GetMethodID(streamIdCls, METHOD_NAME_STREAMIDHOLDER_SET,
                                     METHOD_SIG_STREAMIDHOLDER_SET);
    if (set == nullptr) {
      LOGE("找不到[%s]方法!", METHOD_NAME_STREAMIDHOLDER_SET);
      env->DeleteLocalRef(streamIdCls);
      return ret;
    }

    env->CallVoidMethod(streamIdHolder, set, uStreamId);
    env->DeleteLocalRef(streamIdCls);
  }

  return ret;
}

void Java_com_jxll_vmsdk_VmNet_StopStream(JNIEnv *env, jobject, jint streamId) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }
  LOGI("Java_com_jxll_vmsdk_VmNet_StopStream(%d)", streamId);

  VmNet_StopStream(streamId);
}

void Java_com_jxll_vmsdk_VmNet_SendControl(JNIEnv *env, jobject, jstring fdId, jint channelId,
                                                 jint controlType, jint param1, jint param2) {
  if (g_init.load() != INITED || g_pJavaVM == nullptr) {
    return;
  }

  const char *sFdId = nullptr;
  sFdId = env->GetStringUTFChars(fdId, 0);

  LOGI("Java_com_jxll_vmsdk_VmNet_SendControl(%s, %d, %d, %d, %d)", sFdId, channelId,
       controlType, param1, param2);

  VmNet_SendControl(sFdId, channelId, controlType, param1, param2);
}

} // extern "C"


