/*
 * video2pic.c
 *
 *  Created on: Jul 21, 2013
 *      Author: minfang
 */

#include "video2play.h"
#include <android/log.h>

ANativeWindow* surface;

ANativeWindow_Buffer buffer;
int video2Play(ANativeWindow * s) {
	surface = s;

	// there is not avformat_alloc_context in tutorial
	AVFormatContext *pFormatCtx = avformat_alloc_context();

	//const char * videoPath = "/tmp/s.mp4";
	const char * videoPath = "/sdcard/Sounds/a.ts";

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
			"start of open video:%s\n", videoPath);
	av_register_all();

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
			"av_register_all finish\n");

	// new function
	if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0) {
		__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
				"avformat_open_input fail");
		return -1; // Couldn't open file
	}

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "open finish\n");

	// new function
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
				"avformat_find_stream_info fail");
		return -1; // Couldn't find stream information
	}

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
			"avformat_find_stream_info finish\n");

	//dump_format(pFormatCtx, 0, argv[1], 0);

	int i;
	AVCodecContext *pCodecCtx;
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
			"total size of stream:%d\n", pFormatCtx->nb_streams);
	int videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
				"Didn't find a video stream");
		return -1; // Didn't find a video stream
	}

	// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	AVCodec *pCodec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec
	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "codec id is:%d\n",
			pCodecCtx->codec_id);
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "avcodec_open2 fail");
		return -1; // Could not open codec
	}

	AVFrame * pFrameRGB = avcodec_alloc_frame();

	if (pFrameRGB == NULL)
		return -1;

	int frameFinished;
	AVPacket packet;

	i = 0;

	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			AVFrame *pFrame = avcodec_alloc_frame();
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {
				// Convert the image from its native format to RGB
				if (ANativeWindow_lock(surface, &buffer, NULL) == 0) {
					pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);
					drawScreen(pCodecCtx, pFrame, pFrameRGB);
				}
				ANativeWindow_unlockAndPost(surface);
			}
			av_free(pFrame);

		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);

	}

	av_free(pFrameRGB);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "ok3\n");

	return 0;
}
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[128];
	int y;

	__android_log_print(ANDROID_LOG_DEBUG, "", "w:%d h:%d", width, height);

	return;

}

struct SwsContext * pSWSContext = NULL;
static char c = 5;
static char c1 = 11;
static char c2 = 23;

void drawScreen(AVCodecContext *pCodecCtx, AVFrame * oriPFrame,
		AVFrame *tarFrame) {

	c1 += c2++;

	if (pSWSContext == NULL) {
		pSWSContext = sws_getContext(pCodecCtx->width, pCodecCtx->height,
				pCodecCtx->pix_fmt, buffer.width, buffer.height,
				AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0, 0);
	}

	int numBytes;
	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(AV_PIX_FMT_RGBA, buffer.width, buffer.height);

	avpicture_fill((AVPicture *) tarFrame, buffer.bits, AV_PIX_FMT_RGBA,
			buffer.width, buffer.height);

		sws_scale(pSWSContext, oriPFrame->data, oriPFrame->linesize, 0,
				pCodecCtx->height, tarFrame->data, tarFrame->linesize);

	tarFrame->pts = oriPFrame->pts;

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG",
			"surface w:%d h:%d ,fomat2:%d   %d", buffer.width, buffer.height,
			buffer.format, oriPFrame->pts);
	int i1;
	char * p = buffer.bits;
//
//	for (i1 = 0; i1 < numBytes; i1++) {
//		p[i1] = c + c1;
//		c1 += 23;
//		c += 71;
//	}

}
