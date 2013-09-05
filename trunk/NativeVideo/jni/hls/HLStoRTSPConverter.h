


#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>


#include "config.h"
#include "logjam.h" 
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

#include "libavformat/ffm.h"

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

#include "cmdutils.h"

#include "libavutil/avassert.h"

#define MAX_FILES 100
#define MAX_STREAMS 1024    /* arbitrary sanity check value */

struct AVInputStream;

typedef struct AVOutputStream {
    int file_index;          /* file index */
    int index;               /* stream index in the output file */
    int source_index;        /* AVInputStream index */
    AVStream *st;            /* stream in the output file */
    int encoding_needed;     /* true if encoding needed for this stream */
    int frame_number;
    /* input pts and corresponding output pts
       for A/V sync */
    //double sync_ipts;        /* dts from the AVPacket of the demuxer in second units */
    struct AVInputStream *sync_ist; /* input stream to sync against */
    int64_t sync_opts;       /* output frame counter, could be changed to some true timestamp */ //FIXME look at frame_number
    AVBitStreamFilterContext *bitstream_filters;
    AVCodec *enc;

    /* video only */
    int video_resample;
    AVFrame resample_frame;              /* temporary frame for image resampling */
    struct SwsContext *img_resample_ctx; /* for image resampling */
    int resample_height;
    int resample_width;
    int resample_pix_fmt;
    AVRational frame_rate;

    float frame_aspect_ratio;

    /* forced key frames */
    int64_t *forced_kf_pts;
    int forced_kf_count;
    int forced_kf_index;

    /* audio only */
    int audio_resample;
    ReSampleContext *resample; /* for audio resampling */
    int resample_sample_fmt;
    int resample_channels;
    int resample_sample_rate;
    int reformat_pair;
    AVAudioConvert *reformat_ctx;
    AVFifoBuffer *fifo;     /* for compression: one audio fifo per codec */
    FILE *logfile;

#if CONFIG_AVFILTER
    AVFilterContext *output_video_filter;
    AVFilterContext *input_video_filter;
    AVFilterBufferRef *picref;
    char *avfilter;
    AVFilterGraph *graph;
#endif

   int sws_flags;
} AVOutputStream;

typedef struct AVInputStream {
    int file_index;
    AVStream *st;
    int discard;             /* true if stream data should be discarded */
    int decoding_needed;     /* true if the packets must be decoded in 'raw_fifo' */
    int64_t sample_index;      /* current sample */

    int64_t       start;     /* time when read started */
    int64_t       next_pts;  /* synthetic pts for cases where pkt.pts
                                is not defined */
    int64_t       pts;       /* current pts */
    int is_start;            /* is 1 at the start and after a discontinuity */
    int showed_multi_packet_warning;
    int is_past_recording_time;
#if CONFIG_AVFILTER
    AVFrame *filter_frame;
    int has_filter_frame;
#endif
} AVInputStream;

typedef struct AVInputFile {
    AVFormatContext *ctx;
    int eof_reached;      /* true if eof reached */
    int ist_index;        /* index of first stream in ist_table */
    int buffer_size;      /* current total buffer size */
} AVInputFile;

/* select an input stream for an output stream */
typedef struct AVStreamMap {
    int file_index;
    int stream_index;
    int sync_file_index;
    int sync_stream_index;
} AVStreamMap;

/**
 * select an input file for an output file
 */
typedef struct AVMetaDataMap {
    int  file;      //< file index
    char type;      //< type of metadata to copy -- (g)lobal, (s)tream, (c)hapter or (p)rogram
    int  index;     //< stream/chapter/program number
} AVMetaDataMap;

typedef struct AVChapterMap {
    int in_file;
    int out_file;
} AVChapterMap;



typedef struct HLStoRTSPConverter
{
	int  stopFFMpeg;
	const char *last_asked_format;
	int64_t input_files_ts_offset[MAX_FILES];
	double *input_files_ts_scale[MAX_FILES];
	AVCodec **input_codecs;
	int nb_input_codecs;
	int nb_input_files_ts_scale[MAX_FILES];

	AVFormatContext *output_files[MAX_FILES];
	int nb_output_files;

	/* first item specifies output metadata, second is input */
	AVMetaDataMap (*meta_data_maps)[2];
	int nb_meta_data_maps;
	int metadata_global_autocopy;
	int metadata_streams_autocopy;
	int metadata_chapters_autocopy;

	AVChapterMap *chapter_maps;
	int nb_chapter_maps;

	int *streamid_map;
	int nb_streamid_map;

	int frame_width;
	int frame_height;
	float frame_aspect_ratio;
	enum PixelFormat frame_pix_fmt;
	int frame_bits_per_raw_sample;
	enum AVSampleFormat audio_sample_fmt;
	int max_frames[4];
	AVRational frame_rate;
	float video_qscale;
	uint16_t *intra_matrix;
	uint16_t *inter_matrix;
	const char *video_rc_override_string;
	int video_disable;
	int video_discard;
	char *video_codec_name;
	unsigned int video_codec_tag;
	char *video_language;
	int same_quality;
	int do_deinterlace;
	int top_field_first;
	int me_threshold;
	int intra_dc_precision;
	int loop_input;
	int loop_output;
	int qp_hist;
	#if CONFIG_AVFILTER
	char *vfilters;
	#endif

	int intra_only;
	int audio_sample_rate;
	int64_t channel_layout;

	float audio_qscale;
	int audio_disable;
	int audio_channels;
	char  *audio_codec_name;
	unsigned int audio_codec_tag;
	char *audio_language;

	int subtitle_disable;
	char *subtitle_codec_name;
	char *subtitle_language;
	unsigned int subtitle_codec_tag;

	int data_disable;
	char *data_codec_name;
	unsigned int data_codec_tag;
	float mux_preload;
	float mux_max_delay;

	int64_t recording_time;
	int64_t start_time;
	int64_t recording_timestamp;
	int64_t input_ts_offset;
	int file_overwrite;
	AVDictionary *metadata;
	int do_benchmark;
	int do_hex_dump;
	int do_pkt_dump;
	int do_psnr;
	int do_pass;
	const char *pass_logfilename_prefix;
	int audio_stream_copy;
	int video_stream_copy;
	int subtitle_stream_copy;
	int data_stream_copy;
	int video_sync_method;
	int audio_sync_method;
	float audio_drift_threshold;
	int copy_ts;
	int copy_tb;
	int opt_shortest;
	char *vstats_filename;
	FILE *vstats_file;
	int opt_programid;
	int copy_initial_nonkeyframes;

	int rate_emu;

	int  video_channel;
	char *video_standard;

	int audio_volume;

	int exit_on_error;
	int using_stdin;
	int verbose;
	int run_as_daemon;
	int thread_count;
	int q_pressed;
	int64_t video_size;
	int64_t audio_size;
	int64_t extra_size;
	int nb_frames_dup;
	int nb_frames_drop;
	int input_sync;
	uint64_t limit_filesize;
	int force_fps;
	char *forced_key_frames;

	float dts_delta_threshold;

	int64_t timer_start;

	uint8_t *audio_buf;
	uint8_t *audio_out;
	unsigned int allocated_audio_out_size, allocated_audio_buf_size;

	short *samples;

	AVBitStreamFilterContext *video_bitstream_filters;
	AVBitStreamFilterContext *audio_bitstream_filters;
	AVBitStreamFilterContext *subtitle_bitstream_filters;

	AVOutputStream **output_streams_for_file[MAX_FILES];
	int nb_output_streams_for_file[MAX_FILES];

	AVInputStream *input_streams;
	int         nb_input_streams;
	AVInputFile   *input_files;
	int         nb_input_files;

} HLStoRTSPConverter;


/*class HLStoRTSPConverter 
{
	HLStoRTSPConverter();
	~HLStoRTSPConverter();

};*/

