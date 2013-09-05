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
int video2pic();
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

#endif /* VIDEO2PIC_H_ */
