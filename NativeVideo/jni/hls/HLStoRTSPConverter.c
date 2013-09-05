#include "HLStoRTSPConverter.h"

#define QSCALE_NONE -99999

static const OptionDef options[] = {
    /* main options */
#include "cmdutils_common_opts.h"
    { "f", HAS_ARG, {(void*)opt_format}, "force format", "fmt" },
    { "i", HAS_ARG, {(void*)opt_input_file}, "input file name", "filename" },
    { "y", OPT_BOOL, {(void*)&file_overwrite}, "overwrite output files" },
    { "map", HAS_ARG | OPT_EXPERT, {(void*)opt_map}, "set input stream mapping", "file.stream[:syncfile.syncstream]" },
    { "map_meta_data", HAS_ARG | OPT_EXPERT, {(void*)opt_map_meta_data}, "DEPRECATED set meta data information of outfile from infile",
      "outfile[,metadata]:infile[,metadata]" },
    { "map_metadata", HAS_ARG | OPT_EXPERT, {(void*)opt_map_metadata}, "set metadata information of outfile from infile",
      "outfile[,metadata]:infile[,metadata]" },
    { "map_chapters",  HAS_ARG | OPT_EXPERT, {(void*)opt_map_chapters},  "set chapters mapping", "outfile:infile" },
    { "t", HAS_ARG, {(void*)opt_recording_time}, "record or transcode \"duration\" seconds of audio/video", "duration" },
    { "fs", HAS_ARG | OPT_INT64, {(void*)&limit_filesize}, "set the limit file size in bytes", "limit_size" }, //
    { "ss", HAS_ARG, {(void*)opt_start_time}, "set the start time offset", "time_off" },
    { "itsoffset", HAS_ARG, {(void*)opt_input_ts_offset}, "set the input ts offset", "time_off" },
    { "itsscale", HAS_ARG, {(void*)opt_input_ts_scale}, "set the input ts scale", "stream:scale" },
    { "timestamp", HAS_ARG, {(void*)opt_recording_timestamp}, "set the recording timestamp ('now' to set the current time)", "time" },
    { "metadata", HAS_ARG, {(void*)opt_metadata}, "add metadata", "string=string" },
    { "dframes", OPT_INT | HAS_ARG, {(void*)&max_frames[AVMEDIA_TYPE_DATA]}, "set the number of data frames to record", "number" },
    { "benchmark", OPT_BOOL | OPT_EXPERT, {(void*)&do_benchmark},
      "add timings for benchmarking" },
    { "timelimit", HAS_ARG, {(void*)opt_timelimit}, "set max runtime in seconds", "limit" },
    { "dump", OPT_BOOL | OPT_EXPERT, {(void*)&do_pkt_dump},
      "dump each input packet" },
    { "hex", OPT_BOOL | OPT_EXPERT, {(void*)&do_hex_dump},
      "when dumping packets, also dump the payload" },
    { "re", OPT_BOOL | OPT_EXPERT, {(void*)&rate_emu}, "read input at native frame rate", "" },
    { "loop_input", OPT_BOOL | OPT_EXPERT, {(void*)&loop_input}, "loop (current only works with images)" },
    { "loop_output", HAS_ARG | OPT_INT | OPT_EXPERT, {(void*)&loop_output}, "number of times to loop output in formats that support looping (0 loops forever)", "" },
    { "v", HAS_ARG, {(void*)opt_verbose}, "set ffmpeg verbosity level", "number" },
    { "target", HAS_ARG, {(void*)opt_target}, "specify target file type (\"vcd\", \"svcd\", \"dvd\", \"dv\", \"dv50\", \"pal-vcd\", \"ntsc-svcd\", ...)", "type" },
    { "threads",  HAS_ARG | OPT_EXPERT, {(void*)opt_thread_count}, "thread count", "count" },
    { "vsync", HAS_ARG | OPT_INT | OPT_EXPERT, {(void*)&video_sync_method}, "video sync method", "" },
    { "async", HAS_ARG | OPT_INT | OPT_EXPERT, {(void*)&audio_sync_method}, "audio sync method", "" },
    { "adrift_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT, {(void*)&audio_drift_threshold}, "audio drift threshold", "threshold" },
    { "copyts", OPT_BOOL | OPT_EXPERT, {(void*)&copy_ts}, "copy timestamps" },
    { "copytb", OPT_BOOL | OPT_EXPERT, {(void*)&copy_tb}, "copy input stream time base when stream copying" },
    { "shortest", OPT_BOOL | OPT_EXPERT, {(void*)&opt_shortest}, "finish encoding within shortest input" }, //
    { "dts_delta_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT, {(void*)&dts_delta_threshold}, "timestamp discontinuity delta threshold", "threshold" },
    { "programid", HAS_ARG | OPT_INT | OPT_EXPERT, {(void*)&opt_programid}, "desired program number", "" },
    { "xerror", OPT_BOOL, {(void*)&exit_on_error}, "exit on error", "error" },
    { "copyinkf", OPT_BOOL | OPT_EXPERT, {(void*)&copy_initial_nonkeyframes}, "copy initial non-keyframes" },

    /* video options */
    { "b", HAS_ARG | OPT_VIDEO, {(void*)opt_bitrate}, "set bitrate (in bits/s)", "bitrate" },
    { "vb", HAS_ARG | OPT_VIDEO, {(void*)opt_bitrate}, "set bitrate (in bits/s)", "bitrate" },
    { "vframes", OPT_INT | HAS_ARG | OPT_VIDEO, {(void*)&max_frames[AVMEDIA_TYPE_VIDEO]}, "set the number of video frames to record", "number" },
    { "r", HAS_ARG | OPT_VIDEO, {(void*)opt_frame_rate}, "set frame rate (Hz value, fraction or abbreviation)", "rate" },
    { "s", HAS_ARG | OPT_VIDEO, {(void*)opt_frame_size}, "set frame size (WxH or abbreviation)", "size" },
    { "aspect", HAS_ARG | OPT_VIDEO, {(void*)opt_frame_aspect_ratio}, "set aspect ratio (4:3, 16:9 or 1.3333, 1.7777)", "aspect" },
    { "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_frame_pix_fmt}, "set pixel format, 'list' as argument shows all the pixel formats supported", "format" },
    { "bits_per_raw_sample", OPT_INT | HAS_ARG | OPT_VIDEO, {(void*)&frame_bits_per_raw_sample}, "set the number of bits per raw sample", "number" },
    { "croptop",  HAS_ARG | OPT_VIDEO, {(void*)opt_frame_crop}, "Removed, use the crop filter instead", "size" },
    { "cropbottom", HAS_ARG | OPT_VIDEO, {(void*)opt_frame_crop}, "Removed, use the crop filter instead", "size" },
    { "cropleft", HAS_ARG | OPT_VIDEO, {(void*)opt_frame_crop}, "Removed, use the crop filter instead", "size" },
    { "cropright", HAS_ARG | OPT_VIDEO, {(void*)opt_frame_crop}, "Removed, use the crop filter instead", "size" },
    { "padtop", HAS_ARG | OPT_VIDEO, {(void*)opt_pad}, "Removed, use the pad filter instead", "size" },
    { "padbottom", HAS_ARG | OPT_VIDEO, {(void*)opt_pad}, "Removed, use the pad filter instead", "size" },
    { "padleft", HAS_ARG | OPT_VIDEO, {(void*)opt_pad}, "Removed, use the pad filter instead", "size" },
    { "padright", HAS_ARG | OPT_VIDEO, {(void*)opt_pad}, "Removed, use the pad filter instead", "size" },
    { "padcolor", HAS_ARG | OPT_VIDEO, {(void*)opt_pad}, "Removed, use the pad filter instead", "color" },
    { "intra", OPT_BOOL | OPT_EXPERT | OPT_VIDEO, {(void*)&intra_only}, "use only intra frames"},
    { "vn", OPT_BOOL | OPT_VIDEO, {(void*)&video_disable}, "disable video" },
    { "vdt", OPT_INT | HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)&video_discard}, "discard threshold", "n" },
    { "qscale", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_qscale}, "use fixed video quantizer scale (VBR)", "q" },
    { "rc_override", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_video_rc_override_string}, "rate control override for specific intervals", "override" },
    { "vcodec", HAS_ARG | OPT_VIDEO, {(void*)opt_codec}, "force video codec ('copy' to copy stream)", "codec" },
    { "me_threshold", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_me_threshold}, "motion estimaton threshold",  "threshold" },
    { "sameq", OPT_BOOL | OPT_VIDEO, {(void*)&same_quality},
      "use same quantizer as source (implies VBR)" },
    { "pass", HAS_ARG | OPT_VIDEO, {(void*)opt_pass}, "select the pass number (1 or 2)", "n" },
    { "passlogfile", HAS_ARG | OPT_VIDEO, {(void*)&opt_passlogfile}, "select two pass log file name prefix", "prefix" },
    { "deinterlace", OPT_BOOL | OPT_EXPERT | OPT_VIDEO, {(void*)&do_deinterlace},
      "deinterlace pictures" },
    { "psnr", OPT_BOOL | OPT_EXPERT | OPT_VIDEO, {(void*)&do_psnr}, "calculate PSNR of compressed frames" },
    { "vstats", OPT_EXPERT | OPT_VIDEO, {(void*)&opt_vstats}, "dump video coding statistics to file" },
    { "vstats_file", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_vstats_file}, "dump video coding statistics to file", "file" },
#if CONFIG_AVFILTER
    { "vf", OPT_STRING | HAS_ARG, {(void*)&vfilters}, "video filters", "filter list" },
#endif
    { "intra_matrix", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_intra_matrix}, "specify intra matrix coeffs", "matrix" },
    { "inter_matrix", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_inter_matrix}, "specify inter matrix coeffs", "matrix" },
    { "top", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_top_field_first}, "top=1/bottom=0/auto=-1 field first", "" },
    { "dc", OPT_INT | HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)&intra_dc_precision}, "intra_dc_precision", "precision" },
    { "vtag", HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void*)opt_codec_tag}, "force video tag/fourcc", "fourcc/tag" },
    { "newvideo", OPT_VIDEO, {(void*)opt_new_stream}, "add a new video stream to the current output stream" },
    { "vlang", HAS_ARG | OPT_STRING | OPT_VIDEO, {(void *)&video_language}, "set the ISO 639 language code (3 letters) of the current video stream" , "code" },
    { "qphist", OPT_BOOL | OPT_EXPERT | OPT_VIDEO, { (void *)&qp_hist }, "show QP histogram" },
    { "force_fps", OPT_BOOL | OPT_EXPERT | OPT_VIDEO, {(void*)&force_fps}, "force the selected framerate, disable the best supported framerate selection" },
    { "streamid", HAS_ARG | OPT_EXPERT, {(void*)opt_streamid}, "set the value of an outfile streamid", "streamIndex:value" },
    { "force_key_frames", OPT_STRING | HAS_ARG | OPT_EXPERT | OPT_VIDEO, {(void *)&forced_key_frames}, "force key frames at specified timestamps", "timestamps" },

    /* audio options */
    { "ab", HAS_ARG | OPT_AUDIO, {(void*)opt_bitrate}, "set bitrate (in bits/s)", "bitrate" },
    { "aframes", OPT_INT | HAS_ARG | OPT_AUDIO, {(void*)&max_frames[AVMEDIA_TYPE_AUDIO]}, "set the number of audio frames to record", "number" },
    { "aq", OPT_FLOAT | HAS_ARG | OPT_AUDIO, {(void*)&audio_qscale}, "set audio quality (codec-specific)", "quality", },
    { "ar", HAS_ARG | OPT_AUDIO, {(void*)opt_audio_rate}, "set audio sampling rate (in Hz)", "rate" },
    { "ac", HAS_ARG | OPT_AUDIO, {(void*)opt_audio_channels}, "set number of audio channels", "channels" },
    { "an", OPT_BOOL | OPT_AUDIO, {(void*)&audio_disable}, "disable audio" },
    { "acodec", HAS_ARG | OPT_AUDIO, {(void*)opt_codec}, "force audio codec ('copy' to copy stream)", "codec" },
    { "atag", HAS_ARG | OPT_EXPERT | OPT_AUDIO, {(void*)opt_codec_tag}, "force audio tag/fourcc", "fourcc/tag" },
    { "vol", OPT_INT | HAS_ARG | OPT_AUDIO, {(void*)&audio_volume}, "change audio volume (256=normal)" , "volume" }, //
    { "newaudio", OPT_AUDIO, {(void*)opt_new_stream}, "add a new audio stream to the current output stream" },
    { "alang", HAS_ARG | OPT_STRING | OPT_AUDIO, {(void *)&audio_language}, "set the ISO 639 language code (3 letters) of the current audio stream" , "code" },
    { "sample_fmt", HAS_ARG | OPT_EXPERT | OPT_AUDIO, {(void*)opt_audio_sample_fmt}, "set sample format, 'list' as argument shows all the sample formats supported", "format" },

    /* subtitle options */
    { "sn", OPT_BOOL | OPT_SUBTITLE, {(void*)&subtitle_disable}, "disable subtitle" },
    { "scodec", HAS_ARG | OPT_SUBTITLE, {(void*)opt_codec}, "force subtitle codec ('copy' to copy stream)", "codec" },
    { "newsubtitle", OPT_SUBTITLE, {(void*)opt_new_stream}, "add a new subtitle stream to the current output stream" },
    { "slang", HAS_ARG | OPT_STRING | OPT_SUBTITLE, {(void *)&subtitle_language}, "set the ISO 639 language code (3 letters) of the current subtitle stream" , "code" },
    { "stag", HAS_ARG | OPT_EXPERT | OPT_SUBTITLE, {(void*)opt_codec_tag}, "force subtitle tag/fourcc", "fourcc/tag" },

    /* grab options */
    { "vc", HAS_ARG | OPT_EXPERT | OPT_VIDEO | OPT_GRAB, {(void*)opt_video_channel}, "set video grab channel (DV1394 only)", "channel" },
    { "tvstd", HAS_ARG | OPT_EXPERT | OPT_VIDEO | OPT_GRAB, {(void*)opt_video_standard}, "set television standard (NTSC, PAL (SECAM))", "standard" },
    { "isync", OPT_BOOL | OPT_EXPERT | OPT_GRAB, {(void*)&input_sync}, "sync read on input", "" },

    /* muxer options */
    { "muxdelay", OPT_FLOAT | HAS_ARG | OPT_EXPERT, {(void*)&mux_max_delay}, "set the maximum demux-decode delay", "seconds" },
    { "muxpreload", OPT_FLOAT | HAS_ARG | OPT_EXPERT, {(void*)&mux_preload}, "set the initial demux-decode delay", "seconds" },

    { "absf", HAS_ARG | OPT_AUDIO | OPT_EXPERT, {(void*)opt_bsf}, "", "bitstream_filter" },
    { "vbsf", HAS_ARG | OPT_VIDEO | OPT_EXPERT, {(void*)opt_bsf}, "", "bitstream_filter" },
    { "sbsf", HAS_ARG | OPT_SUBTITLE | OPT_EXPERT, {(void*)opt_bsf}, "", "bitstream_filter" },

    { "apre", HAS_ARG | OPT_AUDIO | OPT_EXPERT, {(void*)opt_preset}, "set the audio options to the indicated preset", "preset" },
    { "vpre", HAS_ARG | OPT_VIDEO | OPT_EXPERT, {(void*)opt_preset}, "set the video options to the indicated preset", "preset" },
    { "spre", HAS_ARG | OPT_SUBTITLE | OPT_EXPERT, {(void*)opt_preset}, "set the subtitle options to the indicated preset", "preset" },
    { "fpre", HAS_ARG | OPT_EXPERT, {(void*)opt_preset}, "set options from indicated preset file", "filename" },
    /* data codec support */
    { "dcodec", HAS_ARG | OPT_DATA, {(void*)opt_codec}, "force data codec ('copy' to copy stream)", "codec" },

    { "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, {(void*)opt_default}, "generic catch all option", "" },
    { NULL, },
};


static HLStoRTSPConverter *mainObj = 0;

void InitMainObj()
{
	mainObj->metadata_global_autocopy   = 1;
	mainObj->metadata_streams_autocopy  = 1;
	mainObj->metadata_chapters_autocopy = 1;
	mainObj->frame_pix_fmt = PIX_FMT_NONE;
	mainObj->audio_sample_fmt = AV_SAMPLE_FMT_NONE;
	mainObj->max_frames[0] = INT_MAX;
	mainObj->max_frames[1] = INT_MAX;
	mainObj->max_frames[2] = INT_MAX;
	mainObj->max_frames[3] = INT_MAX;
	mainObj->top_field_first = -1;
	mainObj->loop_output = AVFMT_NOOUTPUTLOOP;
	mainObj->audio_qscale = QSCALE_NONE;	
	mainObj->mux_preload= 0.5;
	mainObj->mux_max_delay= 0.7;
	mainObj->recording_time = INT64_MAX;
	mainObj->audio_volume = 256;
	mainObj->verbose = 1;
	mainObj->thread_count= 1;
	mainObj->dts_delta_threshold = 10;
}

int main(int argc, char **argv)
{
    int64_t ti;

    if( mainObj != 0 )
    {
	free(mainObj);
	mainObj = 0;
    }
	
    mainObj = (HLStoRTSPConverter*)malloc( sizeof(HLStoRTSPConverter) );
    memset( mainObj, 0, sizeof(HLStoRTSPConverter) );

    InitMainObj();

    mainObj->stopFFMpeg = 0;

    av_log_set_flags(AV_LOG_SKIP_REPEATED);

    if(argc>1 && !strcmp(argv[1], "-d")){
        mainObj->run_as_daemon=1;
        mainObj->verbose=-1;
        av_log_set_callback(log_callback_null);
        argc--;
        argv++;
    }

LOGD("main(): registering all modules");

    avcodec_register_all();
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();

LOGD("main(): registered everything");

#if HAVE_ISATTY
    if(isatty(STDIN_FILENO))
        avio_set_interrupt_cb(decode_interrupt_cb);
#endif

LOGD("main(): initting opts");

    init_opts();

LOGD("main(): initted opts.");

    if(mainObj->verbose>=0)
        show_banner();

LOGD("main(): parsing options");

    /* parse options */
    parse_options(argc, argv, options, mainObj->opt_output_file);

LOGD("main(): parsed options");

    if(mainObj->nb_output_files <= 0 && mainObj->nb_input_files == 0) {
        show_usage();
        LOGE( "Use -h to get full help or, even better, run 'man ffmpeg'\n");
	return 1;
    }

    /* file converter / grab */
    if (mainObj->nb_output_files <= 0) {
        LOGE( "At least one output file must be specified\n");
	return 1;
    }

    if (mainObj->nb_input_files == 0) {
        LOGE( "At least one input file must be specified\n");
	return 1;
    }

LOGD("main(): begin transcode");

    ti = getutime();
    if (transcode(mainObj->output_files, mainObj->nb_output_files, mainObj->input_files, mainObj->nb_input_files,
                  mainObj->stream_maps, mainObj->nb_stream_maps) < 0) {
LOGD("main(): end transcode");
	return 1;
    }
    ti = getutime() - ti;
    if (do_benchmark) {
        int maxrss = getmaxrss() / 1024;
        printf("bench: utime=%0.3fs maxrss=%ikB\n", ti / 1000000.0, maxrss);
    }

    return 0;
}
