/*
 * video2pic.h
 *
 *  Created on: Jul 21, 2013
 *      Author: minfang
 */

#ifndef VIDEO2PIC_H_
#define VIDEO2PIC_H_
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libswscale/swscale.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
int video2Play(ANativeWindow * s);
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
void  drawScreen( AVCodecContext *pCodecCtx,
		AVFrame * oriPFrame, AVFrame *tarFrame) ;

#endif /* VIDEO2PIC_H_ */
