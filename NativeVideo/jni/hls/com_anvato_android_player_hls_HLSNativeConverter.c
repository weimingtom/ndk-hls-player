#include <android/log.h>
#include "logjam.h"
#include "com_anvato_android_player_hls_HLSNativeConverter.h"

#include <stdlib.h>
#include <stdbool.h>

void init();
void stopConversion();
void startSequence( const char* address, int segmentCount, int startSegment );
void endSequence(); 
void newSegment( const char* filename, int segmentNo );
void segmentProgress( const char* filename, int segmentNo, int percentage );
void pauseVideo( double period );
void resumeVideo();
int64_t	getVideoStartPts();
int64_t	getAudioStartPts();
void skipSegment( int segmentNo );
void seek( int startSegmentNo, int segmentCount );
int getCurrentSegment();
int64_t getCurrentPosition(); 

#define exit exit_function_not_allowed
#define LOG_ERROR(message) __android_log_write(ANDROID_LOG_ERROR, "HLS", message)
#define LOG_INFO(message) __android_log_write(ANDROID_LOG_INFO, "HLS", message)
#define EXCEPTION_CODE 256


JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_init(JNIEnv *env, jobject obj)
{
	LOGD("init() called");

	init();
}


JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_startSequence(JNIEnv *env, jobject obj, jstring address, jint segmentCount, jint startSegment)
{
	LOGD("startSequence() called");
	
	const char *adrs = (*env)->GetStringUTFChars(env, address, 0);	
	LOGD("%s", adrs);

	startSequence( adrs, segmentCount, startSegment );
	
	(*env)->ReleaseStringUTFChars(env, address, adrs);
}


JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_endSequence(JNIEnv *env, jobject obj)
{
	LOGD("endSequence() called");
	
	endSequence();
}


JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_newSegment(JNIEnv *env, jobject obj, jstring filename, jint segmentNo )
{
	LOGD("newSegment() called");

	const char *fname = (*env)->GetStringUTFChars(env, filename, 0);	
	LOGD("%s", fname);
	
	newSegment( fname, segmentNo );

	(*env)->ReleaseStringUTFChars(env, filename, fname);
}

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_segmentProgress(JNIEnv *env, jobject obj, jstring filename, jint segmentNo, jint percentage )
{
	//LOGD("segmentProgress() called");

	const char *fname = (*env)->GetStringUTFChars(env, filename, 0);	

	LOGD("%s %d", fname, percentage );
	
	segmentProgress( fname, segmentNo, percentage );

	(*env)->ReleaseStringUTFChars(env, filename, fname);
}

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_Stop(JNIEnv *env, jobject obj)
{
	LOGD("Stop() called");

	stopConversion();
}

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_Pause(JNIEnv *env, jobject obj, jdouble period)
{
	LOGD("Pause() called %f", period);

	pauseVideo( period );
}

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_Resume(JNIEnv *env, jobject obj)
{
	LOGD("Resume() called");

	resumeVideo();
}

JNIEXPORT jlong JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetVideoStartPts(JNIEnv *env, jobject obj)
{
	LOGD("Get video start pts() called");

	return getVideoStartPts();
}

JNIEXPORT jlong JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetAudioStartPts(JNIEnv *env, jobject obj)
{
	LOGD("Get audio start pts() called");

	return getAudioStartPts();
}


JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_skipSegment(JNIEnv *env, jobject obj, jint segmentNo )
{
	LOGD("skipSegment() called");
	
	skipSegment( segmentNo );
}

JNIEXPORT void JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_seek(JNIEnv *env, jobject obj, jint startSegmentNo, jint segmentCount )
{
	LOGD("seek() called %d %d", startSegmentNo, segmentCount );
	
	seek( startSegmentNo, segmentCount );
}

JNIEXPORT jint JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetCurrentSegment(JNIEnv *env, jobject obj )
{
	LOGD("GetCurrentSegment() called" );
	
	return getCurrentSegment( );
}

JNIEXPORT jlong JNICALL Java_com_anvato_android_player_hls_HLSNativeConverter_GetCurrentPosition(JNIEnv *env, jobject obj )
{
	//LOGD("GetCurrentPosition() called" );
	
	return getCurrentPosition( );
}

