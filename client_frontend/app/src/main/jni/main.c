//
// Created by 乔逸凡 on 2019-05-20.
//

#include "com_example_a4over6_MainActivity.h"

JNIEXPORT jstring JNICALL Java_com_example_a4over6_MainActivity_StringFromJNI(JNIEnv *env, jobject thiz) {
    return (*env)->NewStringUTF(env, "hello from JNI");
}