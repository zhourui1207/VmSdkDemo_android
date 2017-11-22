#include <jni.h>
#include "smartconfig/smart_config.h"

extern "C" {

JavaVM *g_pJavaVM = nullptr;

jboolean Java_com_joyware_vmsdk_WifiConfig_config(JNIEnv *env, jclass type, jstring wifiSsid_,
                                                  jstring wifiPwd_, jlong packetIntervalMillis,
                                                  jlong waitTimeMillis, jint encryType,
                                                  jint wepKeyIndex,
                                                  jint wepPwdLen) {

    const char *wifiSsid = env->GetStringUTFChars(wifiSsid_, 0);
    const char *wifiPwd = env->GetStringUTFChars(wifiPwd_, 0);

    bool ret = JWSmart_config(wifiSsid, wifiPwd, packetIntervalMillis, waitTimeMillis, encryType,
                              wepKeyIndex, wepPwdLen);

    env->ReleaseStringUTFChars(wifiSsid_, wifiSsid);
    env->ReleaseStringUTFChars(wifiPwd_, wifiPwd);

    return (jboolean) ret;
}

void Java_com_joyware_vmsdk_WifiConfig_stop(JNIEnv *env, jclass type) {
    JWSmart_stop();
}

} // extern "C"


