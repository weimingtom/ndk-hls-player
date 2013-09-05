LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE  := hls
# These need to be in the right order
FFMPEG_LIBS := $(addprefix jni/ffmpeg/, \
 libavdevice/libavdevice.a \
 libavformat/libavformat.a \
 libavcodec/libavcodec.a \
 libavfilter/libavfilter.a \
 libswscale/libswscale.a \
 libavutil/libavutil.a \
 libpostproc/libpostproc.a )
# ffmpeg uses its own deprecated functions liberally, so turn off that annoying noise
LOCAL_CFLAGS += -g -Ijni/ffmpeg  -Wno-deprecated-declarations 
LOCAL_LDLIBS += -llog -lz $(FFMPEG_LIBS) -landroid
LOCAL_SRC_FILES := video2pic/adptor.c video2pic/video2pic.c
include $(BUILD_SHARED_LIBRARY)

