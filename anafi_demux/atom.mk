
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := anafi_demux
LOCAL_DESCRIPTION := video sink demuxer program
LOCAL_CATEGORY_PATH := multimedia
LOCAL_SRC_FILES := anafi_demux.c
LOCAL_LIBRARIES := \
	libmedia-buffers \
	libpdraw-vsink \
	libulog \
	libvideo-defs \
	libvideo-metadata

include $(BUILD_EXECUTABLE)
