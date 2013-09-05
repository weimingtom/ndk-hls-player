/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <android/log.h>
#include <string.h>
#include <android/rect.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include "video2play.h"
#include <jni.h>

static ANativeWindow* theNativeWindow = NULL;

jint Java_test_hls_ffmpeg_MainActivity_convert(JNIEnv* env, jobject this) {
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Hello from native11");

	jclass clazz = (*env)->GetObjectClass(env, this);
	jmethodID method = (*env)->GetMethodID(env, clazz, "PrintNdkLog",
			"(Ljava/lang/String;)V");

	jstring jstr = (*env)->NewStringUTF(env, "This comes from jni.");
	(*env)->CallVoidMethod(env, this, method, jstr);

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Hello from native9");
	return 0;
}



void Java_test_hls_ffmpeg_MainActivity_destorySurface() {
	// make sure we don't leak native windows
	if (theNativeWindow != NULL) {
		ANativeWindow_release(theNativeWindow);
		theNativeWindow = NULL;
	}
}

// set the surface
void Java_test_hls_ffmpeg_MainActivity_setSurface(JNIEnv *env, jclass clazz,
		jobject surface) {
	// obtain a native window from a Java surface
	Java_test_hls_ffmpeg_MainActivity_destorySurface();
	theNativeWindow = ANativeWindow_fromSurface(env, surface);
}

void Java_test_hls_ffmpeg_MainActivity_test() {


	ANativeWindow_Buffer buffer;
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
					"test1");
	video2Play(theNativeWindow);

	if (ANativeWindow_lock(theNativeWindow, &buffer, NULL) == 0) {

		__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
				"surface w:%d h%d ,fomat:%d", buffer.width, buffer.height,
				buffer.format);

		int w=buffer.width;
		char * p=buffer.bits;
		int x,y;
//		for ( y = 0; y < buffer.height; y++) {
//			int ybasic=y*w;
//			for ( x = 0; x <w ; x++) {
//				int ri=(ybasic+x)*4;
//
//				int gi=ri+1;
//				int bi=gi+1;
//				int ai=bi+1;
//
//				p[ri]=255;
//				p[gi]=0;
//				p[bi]=0;
//				p[ai]=255;
//
//			}
//		}

		ANativeWindow_unlockAndPost(theNativeWindow);
	}

}

