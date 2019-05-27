//
// Created by 乔逸凡 on 2019-05-20.
//

#include "com_example_a4over6_MyVpnService.h"
#include "vpn_main.h"

JNIEXPORT jint JNICALL Java_com_example_a4over6_MyVpnService_vpn_1entry
  (JNIEnv *env, jobject instance, jstring hostName_, jint port, jint commandReadFd, jint responseWriteFd) {
    const char *hostName = env->GetStringUTFChars(hostName_, 0);

    int ret = vpn_main(hostName, port, commandReadFd, responseWriteFd);

    env->ReleaseStringUTFChars(hostName_, hostName);

    return ret;
}