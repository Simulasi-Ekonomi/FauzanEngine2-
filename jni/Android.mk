LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := gpu_validator
LOCAL_SRC_FILES := gpu_main.cpp
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv3
include $(BUILD_SHARED_LIBRARY)
