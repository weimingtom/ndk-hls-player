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
#include <jni.h>
#include "video2pic.h"

jint Java_test_hls_ffmpeg_MainActivity_convert(JNIEnv* env, jobject this) {
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Hello from native41");

	video2pic();
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Hello from native9");
	return 0;
}

// set the surface
void Java_test_hls_ffmpeg_MainActivity_setSurface(JNIEnv *env, jclass clazz,
		jobject surface) {

}

void Java_test_hls_ffmpeg_MainActivity_test() {
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Hello from native5");

	video2pic();
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Hello from native6");
}

void Java_test_hls_ffmpeg_MainActivity_destorySurface() {

}
