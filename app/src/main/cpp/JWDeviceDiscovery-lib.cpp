#include <jni.h>
#include "devicediscovery/device_discovery.h"
#include "util/public/platform.h"

extern "C" {

JavaVM *g_pJavaVM = nullptr;
jobject g_deviceFindCallBack = nullptr;
jmethodID g_callBackMethodId = nullptr;

jclass g_deviceInfoClass = nullptr;
jmethodID g_devceInfoConstructorMethodId = nullptr;

const char *METHOD_NAME_DEVICE_FIND_CALLBACK = "onDeviceFindCallBack";
const char *METHOD_SIG_DEVICE_FIND_CALLBACK = "(Lcom/joyware/vmsdk/JWDeviceInfo;)V";

const char *DEVICE_INFO_CLASS_PATH = "com/joyware/vmsdk/JWDeviceInfo";
const char *DEVICE_INFO_METHOD_NAME_CONSTRUCTOR = "<init>";
//public JWDeviceInfo(byte[] product, byte[] macId, byte obcpVersion, byte obcpCrypt, short
//        obcpPort, int obcpAddr, int netMask, int gateWay, byte[] module, byte[] version,
//byte[] serial, byte[] hardware, byte[] dspVersion, int bootTime, byte[]
//support, short httpPort, byte activated, byte resv1, byte[] resv2)
const char *DEVICE_INFO_METHOD_SIG_CONSTRUCTOR = "([B[BBBSIII[B[B[B[B[BI[BSBB[B)V";

const char *TAG = "JWDD-lib";

void onDeviceFindCallBack(const JWDeviceInfo &deviceInfo) {
    if (g_pJavaVM && g_deviceFindCallBack && g_callBackMethodId && g_deviceInfoClass &&
        g_devceInfoConstructorMethodId) {
        JNIEnv *jniEnv = nullptr;
        if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) != JNI_OK) {
            LOGE(TAG, "onDeviceFindCallBack 获取jniEnv失败！\n");
            return;
        }
        if (!jniEnv) {
            return;
        }
        // 构造java类
        jobject javaDeviceInfo = jniEnv->AllocObject(g_deviceInfoClass);
        if (!javaDeviceInfo) {
            LOGE(TAG, "Alloc java JWDeviceInfo object failed!\n");
        }

        jbyteArray productArray = jniEnv->NewByteArray(4);
        jniEnv->SetByteArrayRegion(productArray, 0, 4, (const jbyte *) deviceInfo.product);
        jbyteArray macIdArray = jniEnv->NewByteArray(8);
        jniEnv->SetByteArrayRegion(macIdArray, 0, 8, (const jbyte *) deviceInfo.macId);
        jbyteArray moduleArray = jniEnv->NewByteArray(32);
        jniEnv->SetByteArrayRegion(moduleArray, 0, 32, (const jbyte *) deviceInfo.module);
        jbyteArray versionArray = jniEnv->NewByteArray(32);
        jniEnv->SetByteArrayRegion(versionArray, 0, 32, (const jbyte *) deviceInfo.version);
        jbyteArray serialArray = jniEnv->NewByteArray(32);
        jniEnv->SetByteArrayRegion(serialArray, 0, 32, (const jbyte *) deviceInfo.serial);
        jbyteArray hardwareArray = jniEnv->NewByteArray(32);
        jniEnv->SetByteArrayRegion(hardwareArray, 0, 32, (const jbyte *) deviceInfo.hardware);
        jbyteArray dspVersionArray = jniEnv->NewByteArray(32);
        jniEnv->SetByteArrayRegion(dspVersionArray, 0, 32, (const jbyte *) deviceInfo.dspVersion);
        jbyteArray supportArray = jniEnv->NewByteArray(8);
        jniEnv->SetByteArrayRegion(supportArray, 0, 8, (const jbyte *) deviceInfo.support);
        jbyteArray resv2Array = jniEnv->NewByteArray(52);
        jniEnv->SetByteArrayRegion(resv2Array, 0, 52, (const jbyte *) deviceInfo.resv2);

        // 调用java类构造函数
        jniEnv->CallVoidMethod(javaDeviceInfo, g_devceInfoConstructorMethodId, productArray,
                               macIdArray, deviceInfo.obcpVersion, deviceInfo.obcpCrypt,
                               deviceInfo.obcpPort, deviceInfo.obcpAddr, deviceInfo.netMask,
                               deviceInfo.gateWay, moduleArray, versionArray, serialArray,
                               hardwareArray, dspVersionArray, deviceInfo.bootTime, supportArray,
                               deviceInfo.httpPort, deviceInfo.activated, deviceInfo.resv1,
                               resv2Array);

        jniEnv->DeleteLocalRef(productArray);
        jniEnv->DeleteLocalRef(macIdArray);
        jniEnv->DeleteLocalRef(moduleArray);
        jniEnv->DeleteLocalRef(versionArray);
        jniEnv->DeleteLocalRef(serialArray);
        jniEnv->DeleteLocalRef(hardwareArray);
        jniEnv->DeleteLocalRef(dspVersionArray);
        jniEnv->DeleteLocalRef(supportArray);
        jniEnv->DeleteLocalRef(resv2Array);

        // 调用接口回调
        jniEnv->CallVoidMethod(g_deviceFindCallBack, g_callBackMethodId, javaDeviceInfo);
        jniEnv->DeleteLocalRef(javaDeviceInfo);  // 释放对象
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jint result = -1;

    if ((vm)->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    //---------------自己需要提前初始化的逻辑--------------------
    // 保存java虚拟机对象
    env->GetJavaVM(&g_pJavaVM);
    // 库被加载时提前初始化
    jclass deviceInfoCls = env->FindClass(DEVICE_INFO_CLASS_PATH);
    if (!deviceInfoCls) {
        LOGE(TAG, "Can't find class [%s]\n", DEVICE_INFO_CLASS_PATH);
        return -1;
    }
    g_deviceInfoClass = (jclass) env->NewGlobalRef(deviceInfoCls);
    if (!g_deviceInfoClass) {
        LOGE(TAG, "Can't new global JWDeviceInfo class\n");
        return -1;
    }
    //g_devceInfoConstructorMethodId是JWDeviceInfo的构造方法（方法名用”<JWDeviceDiscovery_init>”）的jmethodID:
    g_devceInfoConstructorMethodId = env->GetMethodID(g_deviceInfoClass,
                                                      DEVICE_INFO_METHOD_NAME_CONSTRUCTOR,
                                                      DEVICE_INFO_METHOD_SIG_CONSTRUCTOR);
    if (!g_devceInfoConstructorMethodId) {
        LOGE(TAG, "Can't find JWDeviceInfo's constructor method [%s]\n",
             DEVICE_INFO_METHOD_SIG_CONSTRUCTOR);
        return -1;
    }

    JWDeviceDiscovery_init();

    /* success -- return valid version number */
    result = JNI_VERSION_1_6;

    return result;
}

jboolean
Java_com_joyware_vmsdk_DeviceDiscovery_start(JNIEnv *env, jclass type, jobject deviceFindCallBack) {
    jclass deviceFindCallBackCls = env->GetObjectClass(deviceFindCallBack);
    jmethodID callBackMethod = env->GetMethodID(deviceFindCallBackCls,
                                                METHOD_NAME_DEVICE_FIND_CALLBACK,
                                                METHOD_SIG_DEVICE_FIND_CALLBACK);
    if (!callBackMethod) {
        LOGE(TAG, "Can't find [%s] method!", METHOD_SIG_DEVICE_FIND_CALLBACK);
        env->DeleteLocalRef(deviceFindCallBackCls);
        return (jboolean) false;
    }
    env->DeleteLocalRef(deviceFindCallBackCls);

    bool ret = JWDeviceDiscovery_start(onDeviceFindCallBack);
    if (ret) {
        if (g_deviceFindCallBack) {
            env->DeleteGlobalRef(g_deviceFindCallBack);
        }
        g_deviceFindCallBack = env->NewGlobalRef(deviceFindCallBack);
        g_callBackMethodId = callBackMethod;
    }
    return (jboolean) ret;
}

void
Java_com_joyware_vmsdk_DeviceDiscovery_stop(JNIEnv *env, jclass type) {
    JWDeviceDiscovery_stop();
    if (g_deviceFindCallBack) {
        env->DeleteGlobalRef(g_deviceFindCallBack);
        g_deviceFindCallBack = nullptr;
    }
}

void
Java_com_joyware_vmsdk_DeviceDiscovery_clearup(JNIEnv *env, jclass type) {
    JWDeviceDiscovery_clearup();
}

void
Java_com_joyware_vmsdk_DeviceDiscovery_setAutoRequestInterval(JNIEnv *env, jclass type,
                                                              jint intervalSec) {
    JWDeviceDiscovery_setAutoRequestInterval(intervalSec);
}

} // extern "C"


