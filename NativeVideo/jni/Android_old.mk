LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE  := hls
# These need to be in the right order
FFMPEG_LIBS := $(addprefix ffmpeg/, \
 libavdevice/libavdevice.a \
 libavformat/libavformat.a \
 libavcodec/libavcodec.a \
 libavfilter/libavfilter.a \
 libswscale/libswscale.a \
 libavutil/libavutil.a \
 libpostproc/libpostproc.a )
# ffmpeg uses its own deprecated functions liberally, so turn off that annoying noise
LOCAL_CFLAGS += -g -Iffmpeg -Ihls -Wno-deprecated-declarations 
LOCAL_LDLIBS += -llog -lz $(FFMPEG_LIBS)
LOCAL_SRC_FILES := hls/com_anvato_android_player_hls_HLSNativeConverter.c hls/converter.c hls/ts_parser.c
include $(BUILD_SHARED_LIBRARY)

