LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := hellojni
LOCAL_SRC_FILES := main.cpp proto.cpp vpn.cpp
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
