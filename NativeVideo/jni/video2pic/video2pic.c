/*
 * video2pic.c
 *
 *  Created on: Jul 21, 2013
 *      Author: minfang
 */

#include "video2pic.h"
#include <android/log.h>
int video2pic() {
	// there is not avformat_alloc_context in tutorial
	AVFormatContext *pFormatCtx = avformat_alloc_context();

	//const char * videoPath = "/tmp/s.mp4";
	const char * videoPath = "/sdcard/Sounds/a.ts";

	printf("start of open video:%s\n", videoPath);
	av_register_all();

	printf("av_register_all finish\n");

	// new function
	if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0) {
		printf("avformat_open_input fail");
		return -1; // Couldn't open file
	}

	printf("open finish\n");

	// new function
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("avformat_find_stream_info fail");
		return -1; // Couldn't find stream information
	}

	printf("avformat_find_stream_info finish\n");

	//dump_format(pFormatCtx, 0, argv[1], 0);

	int i;
	AVCodecContext *pCodecCtx;
	printf("total size of stream:%d\n", pFormatCtx->nb_streams);
	int videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		printf("Didn't find a video stream");
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
	printf("codec id is:%d\n", pCodecCtx->codec_id);
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("avcodec_open2 fail");
		return -1; // Could not open codec
	}



	AVFrame * pFrameRGB = avcodec_alloc_frame();

	if (pFrameRGB == NULL)
		return -1;

	uint8_t *buffer;
	int numBytes;
	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
			pCodecCtx->height);

	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	avpicture_fill((AVPicture *) pFrameRGB, buffer, AV_PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);

	int frameFinished;
	AVPacket packet;

	i = 0;

	struct SwsContext * pSWSContext = sws_getContext(pCodecCtx->width,
			pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
			pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, 0, 0, 0);
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			AVFrame *pFrame = avcodec_alloc_frame();
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {
				// Convert the image from its native format to RGB
				pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);
				sws_scale(pSWSContext, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameRGB->data,
						pFrameRGB->linesize);

				pFrameRGB->pts = pFrame->pts;
				// Save the frame to disk
				if (++i <= 5000)
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,
							i);


			}
			av_free(pFrame);

		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	av_free(buffer);
	av_free(pFrameRGB);



	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	printf("ok\n");

	return 0;
}
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[128];
	int y;

	// Open file
	sprintf(szFilename, "/sdcard/Sounds/frame_%d.ppm", pFrame->pts);

	__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "ppm file name:%s",
			szFilename);

	return;

	pFile = fopen(szFilename, "wb");
	if (pFile == NULL){
		__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "fail to open:%s for writing",
					szFilename);
		return;
	}

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y < height; y++)
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}
