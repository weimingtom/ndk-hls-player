/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_anvato_android_player_hls_HLSNativeConverter */

#ifndef _Included_com_anvato_android_player_hls_HLSNativeConverter
#define _Included_com_anvato_android_player_hls_HLSNativeConverter
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_init(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_startSequence(JNIEnv *, jobject, jstring, jint, jint);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_endSequence(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_newSegment(JNIEnv *, jobject, jstring, jint);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_skipSegment(JNIEnv *, jobject, jint);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_seek(JNIEnv *, jobject, jint, jint);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_segmentProgress(JNIEnv *, jobject, jstring, jint, jint);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_Stop(JNIEnv *, jobject);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_Pause(JNIEnv *, jobject, jdouble);

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_Resume(JNIEnv *, jobject);

JNIEXPORT jlong JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetVideoStartPts(JNIEnv *, jobject);

JNIEXPORT jlong JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetAudioStartPts(JNIEnv *, jobject);

JNIEXPORT jint JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetCurrentSegment(JNIEnv *, jobject);

JNIEXPORT jlong JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetCurrentPosition(JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif