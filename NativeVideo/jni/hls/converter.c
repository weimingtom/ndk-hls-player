#include "logjam.h" 

#include "config.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/audioconvert.h"
#include "libavutil/audioconvert.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/colorspace.h"
#include "libavutil/fifo.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/dict.h"
#include "libavutil/pixdesc.h"
#include "libavutil/avstring.h"
#include "libavutil/libm.h"
#include "libavformat/os_support.h"

#include "libavformat/ffm.h" // not public API

#if CONFIG_AVFILTER
# include "libavfilter/avcodec.h"
# include "libavfilter/avfilter.h"
# include "libavfilter/avfiltergraph.h"
# include "libavfilter/vsink_buffer.h"
# include "libavfilter/vsrc_buffer.h"
#endif

#if HAVE_SYS_RESOURCE_H
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#elif HAVE_GETPROCESSTIMES
#include <windows.h>
#endif
#if HAVE_GETPROCESSMEMORYINFO
#include <windows.h>
#include <psapi.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_TERMIOS_H
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#elif HAVE_KBHIT
#include <conio.h>
#endif
#include <time.h>
#include "pthread.h"


#define MAX_SEGMENT_COUNT 1000

struct t_converter
{
	AVFormatContext 		*fmt_ctx_in;
	AVFormatContext 		*fmt_ctx_out;
	AVStream 				*audio_st_out;
	AVStream				*video_st_out;
	
	double					audioTimeBase;
	double					videoTimeBase;
	int						audioIndex;
	int						videoIndex;
		
	int 					isOutputReady;
	int 					curSegmentNo;
	int						startSegmentNo;
	int						segmentCount;
	int						currentSegmentNo;
	int64_t					currentPosition;
	double					audioSampleRate;						
	int						isSegmentReady[MAX_SEGMENT_COUNT];
	char					segmentFilenames[MAX_SEGMENT_COUNT][500];
	
	int						stop;
	int						pause;
	int						seek;
	long					pausePeriod;
	int64_t					videoStartPts;
	int64_t					audioStartPts;

};

static struct t_converter sConverter;

void init() 
{
	int i = 0;
	
	avcodec_init();
	avcodec_register_all();
	av_register_all();
	
	sConverter.fmt_ctx_in = 0;
	sConverter.fmt_ctx_out = 0;
	sConverter.audio_st_out = 0;
	sConverter.video_st_out = 0;
	sConverter.stop = 0;
	sConverter.currentSegmentNo = 0;
	
	for( i = 0; i < MAX_SEGMENT_COUNT; i++ )
	{
		sConverter.isSegmentReady[i] = 0;
	}
}


void resetConverter( const char* address )
{
	int i = 0;
	LOGD ( "Before free fmt_ctx_in" );
	if( sConverter.fmt_ctx_in != 0 )
	{
		avformat_free_context( sConverter.fmt_ctx_in );
		sConverter.fmt_ctx_in = 0;
	}
	LOGD ( "After free fmt_ctx_in" );
	
	if( sConverter.fmt_ctx_out != 0 )
	{
		avformat_free_context( sConverter.fmt_ctx_out );
		sConverter.fmt_ctx_out = 0;
	}
	LOGD ( "After free fmt_ctx_out" );
	
	avformat_alloc_output_context2( &sConverter.fmt_ctx_out, NULL, "rtsp", address ); 	//avformat_alloc_context();
	if ( !sConverter.fmt_ctx_out )
	{
	    LOGD ( "Memory error\n" );	
	    return;
	}
	
	sConverter.isOutputReady = 0;
	sConverter.pause = 0;
	sConverter.videoStartPts = 0;
	sConverter.audioStartPts = 0;
	sConverter.seek = 0;

	for( i = 0; i < MAX_SEGMENT_COUNT; i++ )
	{
		sConverter.isSegmentReady[i] = 0;
	}
}


static AVStream *add_output_stream(AVFormatContext *output_format_context, AVStream *input_stream) {
    AVCodecContext *input_codec_context;
    AVCodecContext *output_codec_context;
    AVStream *output_stream;

    output_stream = av_new_stream(output_format_context, input_stream->id);
    if (!output_stream) {
   	    LOGD("Could not allocate stream");
        return NULL;
    }
    output_stream->id = 0;
    output_stream->time_base = input_stream->time_base; 

    input_codec_context = input_stream->codec;
    output_codec_context = output_stream->codec;

    output_codec_context->codec_id = input_codec_context->codec_id;
    output_codec_context->codec_type = input_codec_context->codec_type;
    output_codec_context->codec_tag = input_codec_context->codec_tag;
    output_codec_context->bit_rate = input_codec_context->bit_rate;
    output_codec_context->extradata = av_mallocz(input_codec_context->extradata_size);
    memcpy(output_codec_context->extradata, input_codec_context->extradata, input_codec_context->extradata_size );
    output_codec_context->extradata_size = input_codec_context->extradata_size;

    /*if(av_q2d(input_codec_context->time_base) * input_codec_context->ticks_per_frame > av_q2d(input_stream->time_base) && av_q2d(input_stream->time_base) < 1.0/1000) {
        output_codec_context->time_base = input_codec_context->time_base;
        //output_codec_context->time_base.num *= input_codec_context->ticks_per_frame;
    }
    else {*/
        output_codec_context->time_base = input_stream->time_base;
    //}
    
    LOGD ( "Intput Time base %d %d \n", input_stream->time_base.den, input_stream->time_base.num );
    LOGD ( "Output Time base %d %d \n", output_codec_context->time_base.den, output_codec_context->time_base.num );

    switch (input_codec_context->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            output_codec_context->channel_layout = input_codec_context->channel_layout;
            output_codec_context->sample_rate = input_codec_context->sample_rate;
            output_codec_context->channels = input_codec_context->channels;
            output_codec_context->frame_size = input_codec_context->frame_size;
            if ((input_codec_context->block_align == 1 && input_codec_context->codec_id == CODEC_ID_MP3) || input_codec_context->codec_id == CODEC_ID_AC3) {
                output_codec_context->block_align = 0;
            }
            else {
                output_codec_context->block_align = input_codec_context->block_align;
            }
            //av_set_pts_info(output_stream, 32,  1, output_codec_context->sample_rate);
            //output_codec_context->time_base.den = output_codec_context->sample_rate;
            //output_codec_context->time_base.num = 1;  
            sConverter.audioTimeBase =  (double)output_codec_context->time_base.den / (double)output_codec_context->time_base.num;
            //sConverter.audioTimeBase = output_codec_context->sample_rate; 
            sConverter.audioSampleRate = output_codec_context->sample_rate; 
            LOGD( "Audio time base %f", sConverter.audioTimeBase );
            break;
        case AVMEDIA_TYPE_VIDEO: 
            output_codec_context->pix_fmt = input_codec_context->pix_fmt;
            output_codec_context->width = input_codec_context->width;
            output_codec_context->height = input_codec_context->height;
            output_codec_context->has_b_frames = input_codec_context->has_b_frames; 
            output_codec_context->profile = input_codec_context->profile;
            output_codec_context->level = input_codec_context->level;  
   
            if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
                output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }            
            sConverter.videoTimeBase =  (double)output_codec_context->time_base.den / (double)output_codec_context->time_base.num;
            LOGD( "Video time base %f", sConverter.videoTimeBase );
            break;    
    default:  
        break; 
    }

    return output_stream;
}


int openInput( const char* filename )
{
	AVInputFormat *infmt = 0;
	int i = 0;
	
	if( sConverter.fmt_ctx_in != 0 )
	{
		avformat_free_context( sConverter.fmt_ctx_in );
		sConverter.fmt_ctx_in = 0;
	}
	
	if ( avformat_open_input ( &(sConverter.fmt_ctx_in), filename, infmt, 0 ) )
	{
		LOGD ( "Cant open input file %s\n", filename );
		return -1;
	}
	
    LOGD ( "open input file success \n" );
			
	if ( av_find_stream_info ( sConverter.fmt_ctx_in ) < 0 )
	{
	    LOGD ( "Cant find stream info [dec_openfile]\n" );
		return -1;
	}    	
    
	
	if( sConverter.isOutputReady == 0 )
	{	
		for ( i=0;i<sConverter.fmt_ctx_in->nb_streams;i++ )
		{
			AVCodecContext *cdc_ctx = sConverter.fmt_ctx_in->streams[i]->codec;
			if ( !cdc_ctx )
				continue;
				
				LOGD ( "Stream info found\n" );	
	
			if ( cdc_ctx->codec_type == AVMEDIA_TYPE_AUDIO )
			{
				LOGD ( "Audio stream found %d %d %d\n", cdc_ctx->channels, cdc_ctx->sample_rate, cdc_ctx->codec_id );
				sConverter.fmt_ctx_out->oformat->audio_codec = cdc_ctx->codec_id;
			
				sConverter.audio_st_out = add_output_stream(sConverter.fmt_ctx_out, sConverter.fmt_ctx_in->streams[i]);
				sConverter.audioIndex = i;
			}
	
			if ( cdc_ctx->codec_type == AVMEDIA_TYPE_VIDEO )
			{
				LOGD ( "Video stream found %d %d %d\n", cdc_ctx->width, cdc_ctx->height, cdc_ctx->codec_id );
				sConverter.fmt_ctx_out->oformat->video_codec = cdc_ctx->codec_id;
			
				sConverter.video_st_out = add_output_stream(sConverter.fmt_ctx_out, sConverter.fmt_ctx_in->streams[i]);
				sConverter.videoIndex = i;
			}
		}
		
		//Find start pts
		AVPacket pkt;
		av_init_packet ( &pkt );
		int video_found = 0;
		int audio_found = 0;
		
		while ( av_read_frame(sConverter.fmt_ctx_in, &pkt) >=0 )
		{
			if( audio_found == 1 && video_found == 1 )
			{
				break;
			}
			//LOGD ( "DTS %lld, %d \n", pkt.dts, pkt.stream_index );
			if( audio_found == 0 )
			{
				if( pkt.stream_index == sConverter.audioIndex )
				{
					sConverter.audioStartPts = (double)pkt.pts * ( sConverter.audioSampleRate / sConverter.videoTimeBase );
					LOGD ( "Audio start pts %lld, %d %d\n", sConverter.audioStartPts, pkt.stream_index, sConverter.audioIndex );
					audio_found = 1;
				}
			}
			
			if( video_found == 0 )
			{
				if( pkt.stream_index == sConverter.videoIndex )
				{
					sConverter.videoStartPts = pkt.pts;
					LOGD ( "Video start pts %lld %d %d\n", pkt.pts, pkt.stream_index, sConverter.videoIndex );
					video_found = 1;
				}
			
			} 
		}
		
		av_seek_frame( sConverter.fmt_ctx_in, sConverter.audioIndex, 0, AVSEEK_FLAG_BYTE);
		av_seek_frame( sConverter.fmt_ctx_in, sConverter.videoIndex, 0, AVSEEK_FLAG_BYTE);
		
		LOGD ( "Connect rtsp server\n" );	
		if ( avformat_write_header ( sConverter.fmt_ctx_out, NULL ) != 0 )
		{
			LOGD ( "Could not init the output '%s'\n", "rtsp" );
			return -1;
		}
		LOGD ( "Connect rtsp server OK '%s'\n", "rtsp" );
		sConverter.isOutputReady = 1;
	
	
		/*LOGD ( "Connect rtsp server\n" );	
		if ( avformat_write_header ( sConverter.fmt_ctx_out, NULL ) != 0 )
		{
			LOGD ( "Could not init the output '%s'\n", "rtsp" );
			return;
		}
		LOGD ( "Connect rtsp server OK '%s'\n", "rtsp" );
		
		sConverter.isOutputReady = 1;*/
	}
	
	return 0;
}

pthread_mutex_t seqMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pauseMutex = PTHREAD_MUTEX_INITIALIZER;

void startSequence( const char* address, int segmentCount, int startSegment )
{
	//pthread_mutex_lock( &seqMutex );
	int i = 0;
	int j = 0;
	int playedSize = 0;
	double dts = 0;
	double startTime = -1;
	double currTime = 0;
	double lastCurrTime = 0;
	double currTimeOffset = 0;
	double pauseStart = 0;
	double pauseEnd = 0;
	int	keyFound = 1;
	int sWaitCount = 0;
	long pausePeriod = 0;
	int64_t timestampShift = 0;
	
	resetConverter( address );

	sConverter.stop = 0;
	sConverter.pausePeriod = 0;
	sConverter.seek = 0;
	sConverter.currentSegmentNo = 0;
	sConverter.currentPosition = 0;
	sConverter.startSegmentNo = startSegment;
	sConverter.segmentCount = segmentCount;
	
	for( i = sConverter.startSegmentNo; i < sConverter.segmentCount; i++ )
	{
		LOGD ( "is Segment Ready %d\n", i  );
		sWaitCount = 0;
		while( sConverter.isSegmentReady[i] == 0 )
		{
			if( sConverter.stop == 1 )
			{
				//pthread_mutex_unlock( &seqMutex );
				/*if( sConverter.fmt_ctx_out != 0 )
				{
					avformat_free_context( sConverter.fmt_ctx_out );
					sConverter.fmt_ctx_out = 0;
				}*/
				return;
			}
			usleep(5000);
			sWaitCount++;
			if( sWaitCount > 8000 ) 
			{
				LOGD ( "SEGMENT READY WAIT IS OVER 8 SEC !!!!" );
				return;
			}
		}
		
		if(	sConverter.isSegmentReady[i] == 2 ) 
		{
			continue;
		}
		
		LOGD ( "segment is ready\n" );
		sConverter.currentSegmentNo = i;
		
		if( openInput( sConverter.segmentFilenames[i] ) < 0 )
		{
			break;
		}
		//usleep( 3 * 1000000 );
		
		AVPacket pkt;
		av_init_packet ( &pkt );
		playedSize = 0;
		
		while ( av_read_frame(sConverter.fmt_ctx_in, &pkt) >=0 )
		{
			
			//if( sConverter.pause == 1 )
			if( sConverter.pausePeriod != 0 && startTime != -1 )
			{
				LOGD ( "Pause started\n" );
				pauseStart = (double)clock() / CLOCKS_PER_SEC;
				if( sConverter.pausePeriod == -1 ) 
				{
					while( sConverter.pausePeriod == -1 && sConverter.stop != 1 )
					{
						if( sConverter.seek == 1 )
						{
							break;
						}
						usleep( 20000 );
					}
				}
				else
				{
					pausePeriod = sConverter.pausePeriod;
					sConverter.pausePeriod = 0;
					
					//pthread_mutex_lock( &pauseMutex );
					//pthread_mutex_unlock( &pauseMutex );
					for( j = 0; j < pausePeriod/10; j++ )
					{
						if( sConverter.seek == 1 )
						{
							LOGD ( "breaking for seek\n" );
							break;
						}
						usleep( 10000 );
						
					}
					
					//usleep( pausePeriod * 1000 );
				}
				pauseEnd = (double)clock() / CLOCKS_PER_SEC;
				startTime += pauseEnd - pauseStart;
				LOGD ( "Pause ended\n" );
			}
			
			if( sConverter.stop == 1 )
			{
				//pthread_mutex_unlock( &seqMutex );
				return;
			}
			
			if( sConverter.seek == 1 )
			{
				LOGD ( "breaking for seek\n" );
				break;
			}
			
			dts = 0;
			if( pkt.stream_index == sConverter.audioIndex )
			{
				dts = pkt.dts / sConverter.audioTimeBase;
				//LOGD ( "Audio Packet pts %lld\n", (pkt.pts) );
			}
			else if( pkt.stream_index == sConverter.videoIndex )
			{
				dts = pkt.dts / sConverter.videoTimeBase;
				//LOGD ( "Video Packet pts %lld\n", (pkt.pts) );
				if( keyFound == 0 )
				{
					if( pkt.flags & AV_PKT_FLAG_KEY )
					{
						LOGD ( "Key Found\n" );
						keyFound = 1;
					}
					else
					{
						LOGD ( "Key not found\n" );
					}
				}
			}
			else
			{
				continue;
			}
				
			//LOGD ( "DTS %f\n", dts );
			if( startTime == -1)
			{
				startTime = (double)clock() / CLOCKS_PER_SEC - dts;
				LOGD ( "Start Time is %f\n", startTime );
			}
			lastCurrTime = currTime;
			currTime = (double)clock() / CLOCKS_PER_SEC - startTime + currTimeOffset;
									
			
			if( currTime + 4200 < lastCurrTime ) 
			{
				LOGD ( "overflow of dts %f\n", currTime );
				currTimeOffset += 4294.967;
				currTime += currTimeOffset;
			}
			
			//LOGD ( "packet read %f %f\n", dts, currTime );
			
			if( currTime + 2 < dts )
			{
				//LOGD ( "wait packet %f\n", ( dts - currTime ) );
				if( ( dts - currTime - 2 ) > 3 ){
					LOGD ( "TOOOOO LOOOOONNNGGGGGGGG TOOOO LOOONNNNGGGG SLLEEEEPPP %f\n", ( dts - currTime ) );
					startTime -= ( dts - currTime );
					timestampShift += -( dts - currTime );
					LOGD( "dts %f", dts );
					LOGD( "currTime %f", currTime );
					LOGD( "Timestamp Shift %f", timestampShift* sConverter.videoTimeBase );
				}
				else
				{
					usleep( ( dts - currTime -2 ) * 1000000 );
				}
			}
			else if( currTime > dts + 100 )
			{
				LOGD ( "EEEEEEERRRRRRR  Something is wrong %f %f\n", dts,  currTime );
				startTime -= ( dts - currTime );
				timestampShift -= ( dts - currTime );
			}
			else 
			{
				usleep( 1000 );
			}
			
			if( keyFound == 1 )
			{			
				if( pkt.stream_index == sConverter.audioIndex )
				{
					//LOGD ( "Audio Timestampshift %f\n", timestampShift * sConverter.audioTimeBase );
					pkt.pts += timestampShift * sConverter.audioTimeBase;
					pkt.dts += timestampShift * sConverter.audioTimeBase;
				}
				else if( pkt.stream_index == sConverter.videoIndex )
				{
					//LOGD ( "Video Timestampshift %f\n", timestampShift * sConverter.audioTimeBase );
					pkt.pts += timestampShift * sConverter.videoTimeBase;
					pkt.dts += timestampShift * sConverter.videoTimeBase;
					
					sConverter.currentPosition = pkt.pts - sConverter.videoStartPts;
				}
				
				//LOGD ( "Packet pts %lld\n", (pkt.pts) );
				av_interleaved_write_frame(sConverter.fmt_ctx_out, &pkt);        
			}
		}
		
		if( sConverter.seek == 1 )
		{
			sConverter.seek = 0;
			i = sConverter.startSegmentNo-1;
			LOGD ( "starting from seek position\n" );
		}
		else
		{
			sConverter.isSegmentReady[i] = 0;
			remove( sConverter.segmentFilenames[i] );
		
			LOGD ( "segment conversion ended %d %f sec\n", i, currTime );
		}
	}
	
	/*if( sConverter.fmt_ctx_out != 0 )
	{
		avformat_free_context( sConverter.fmt_ctx_out );
		sConverter.fmt_ctx_out = 0;
	}*/
//	usleep( 4 * 1000000 );  
	//pthread_mutex_unlock( &seqMutex );
}


int getCurrentSegment()
{
	LOGD("Current Segment No %d", sConverter.currentSegmentNo );
	return sConverter.currentSegmentNo;
}

int64_t getCurrentPosition()
{
	//LOGD ( "getCurrentPosition %lld", sConverter.currentPosition );
	return sConverter.currentPosition/90;  
}

void endSequence()
{ 
	
}

int64_t getVideoStartPts()
{
	return sConverter.videoStartPts; 
}

int64_t getAudioStartPts()
{
	return sConverter.audioStartPts; 
}

void seek( int startSegmentNo, int segmentCount ) 
{
	sConverter.seek = 1;
	sConverter.startSegmentNo = startSegmentNo;
	sConverter.segmentCount = segmentCount;
	
	LOGD ( "Seek: startSegment %d segmentCount %d", sConverter.startSegmentNo, sConverter.segmentCount );
}

void newSegment( const char* filename, int segmentNo )
{
	LOGD ( " New segment %s %d", filename, segmentNo );
	sConverter.isSegmentReady[segmentNo] = 1;
	strcpy( sConverter.segmentFilenames[segmentNo], filename );
}

void skipSegment( int segmentNo ) 
{
	LOGD ( " Skip segment %d", segmentNo );
	sConverter.isSegmentReady[segmentNo] = 2;
}


void segmentProgress( const char* filename, int segmentNo, int percentage )
{
	unsigned int i=0;
	
	/*if( percentage >= 50  && sConverter.isSegmentReady[segmentNo] == 0 )
	{
		LOGD ( "segment progress filename %s", filename );
		sConverter.isSegmentReady[segmentNo] = 1;
		strcpy( sConverter.segmentFilenames[segmentNo], filename );
	}*/
}

void pauseVideo( double period ) 
{
	//if( sConverter.pause == 0 )
	//{
		//pthread_mutex_lock( &pauseMutex );
		sConverter.pausePeriod = (long)period;
		//sConverter.pause = 1;
	//}
}


void resumeVideo() 
{
	//if( sConverter.pause == 1 )
	//{
	//	pthread_mutex_unlock( &pauseMutex );
	//	sConverter.pause = 0;
	//}
	sConverter.pausePeriod = 0;
}

void stopConversion()
{
	if( sConverter.pause == 1 )
	{
		resumeVideo();
	}
	sConverter.stop = 1;
	sConverter.pausePeriod = 0;
}
