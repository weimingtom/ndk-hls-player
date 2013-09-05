#ifndef T_MPEGTS_PARSER_H_
#define T_MPEGTS_PARSER_H_


#define TS_PACKET_LEN 187
#define MAX_ES 256
#define MAX_PROGS 16

typedef struct 
{
	unsigned char		transport_error_indicator;
	unsigned char		payload_start_indicator;
	unsigned char		transport_priority;
	unsigned int 		PID;
	unsigned char		transport_scrambling_control;
	unsigned char		adaption_field_control;
	unsigned char		continuity_counter;
}t_tsheader;

typedef struct 
{
	unsigned char	stream_type;
	unsigned int	pid;
}t_pmt_es_info;

typedef struct 
{
	unsigned int	number;
	unsigned int	pid;
}t_pat_prog_info;

typedef struct 
{
	unsigned char		field_lenght;
	unsigned char		discontinuity_indicator;
	unsigned char		random_access_indicator;
	unsigned char		es_priority_indicator;
	unsigned char		pcr_flag;
	unsigned char		opcr_flag;
	unsigned char		splicing_point_flag;
	unsigned char		transport_private_data_flag;
	unsigned char		af_ext_flag;

	long int			pcr;
	long int			opcr;
	unsigned char		splice_countdown;
}t_adaptation_field;


typedef struct 
{
	unsigned char		table_id; // should be 0
	unsigned char		section_syntax_indicator; // should be 1. O means short version.
	unsigned int		section_lenght;
	unsigned int		ts_id;
	unsigned int 		version_number;
	unsigned char		current_next_indicator;
	unsigned char		section_number;
	unsigned char		last_section_number;

	unsigned int 		prog_count;
	t_pat_prog_info 	prog_infos[MAX_PROGS];
}t_pat;

typedef struct 
{
	unsigned char		table_id; // should be 2
	unsigned char		section_syntax_indicator; // should be 1. O means short version.
	unsigned int		section_lenght;
	unsigned int		prog_num;
	unsigned int 		version_number;
	unsigned char		current_next_indicator;
	unsigned char		section_number; // should be 0
	unsigned char		last_section_number; // should be 0
	unsigned int 		pcr_pid;
	unsigned int 		prog_info_lenght;

	unsigned int		es_count;
	t_pmt_es_info 		es_infos[MAX_ES];
}t_pmt;

typedef struct 
{
	unsigned int		start_code; // should be 0x000001
	unsigned char		id;
	unsigned int		packet_lenght;
	unsigned char		scr_control;
	unsigned char		priority;
	unsigned char		data_alignment;
	unsigned char		copyright;
	unsigned char		original;
	unsigned char		pts_dts_flag;
	unsigned char		escr_flag;
	unsigned char		es_rate_flag;
	unsigned char		dsm_trick_mode_flag;
	unsigned char		add_copy_info_flag;
	unsigned char		crc_flag;
	unsigned char		extension_flag;
	unsigned char		header_lenght;

	long int			pts;
	long int			dts;

}t_pes;

typedef void t_mpegts_pat_cb( void *arg, unsigned int prog_cnt, const t_pat_prog_info* prog_infos );
typedef void t_mpegts_pmt_cb( void *arg, unsigned int es_cnt, const t_pmt_es_info* es_infos );
typedef void t_mpegts_es_cb( void *arg, unsigned int pid, int pyl_start_ind, long int pts,  const char* packetdata );
typedef void t_mpegts_pcr_cb( void *arg, long int pcr );

typedef struct
{
	FILE *file;

	unsigned char 		curr_pkt[TS_PACKET_LEN];
	t_tsheader 			curr_tsheader;
	unsigned int		read_pos;

	t_adaptation_field  adaptation_field;

	t_pat				pat;
	t_mpegts_pat_cb 	*pat_cb_func;
	void 				*pat_cb_arg;

	unsigned int 		pmt_pid;
	t_pmt				pmt;
	t_mpegts_pmt_cb 	*pmt_cb_func;
	void 				*pmt_cb_arg;

	t_mpegts_pcr_cb		*pcr_cb_func;
	void				*pcr_cb_arg;

	t_pes				pes;

	int					es_cb_count;
	t_mpegts_es_cb 		*es_cb_funcs[MAX_ES];
	void				*es_cb_args[MAX_ES];
	unsigned int   		es_cb_pids[MAX_ES];
} t_mpegts_parser;



t_mpegts_parser *mpp_new( const char *file );

void mpp_delete( t_mpegts_parser *mpp );

void mpp_set_pat_cb( t_mpegts_parser *mpp, t_mpegts_pat_cb *cb, void *arg  );
void mpp_set_pmt_cb( t_mpegts_parser *mpp, t_mpegts_pmt_cb *cb, void *arg, unsigned int prog_pid  );
void mpp_add_es_cb( t_mpegts_parser *mpp, t_mpegts_es_cb *cb, void *arg, unsigned int es_pid  );
void mpp_set_pcr_cb( t_mpegts_parser *mpp, t_mpegts_pcr_cb *cb, void *arg );

void mpp_clear_pat_cb( t_mpegts_parser *mpp );
void mpp_clear_pmt_cb( t_mpegts_parser *mpp );
void mpp_clear_es_cb( t_mpegts_parser *mpp );
void mpp_clear_pcr_cb( t_mpegts_parser *mpp );
void mpp_remove_es_cb( t_mpegts_parser *mpp, unsigned int es_pid );

int mpp_parse_next_pkt( t_mpegts_parser *mpp );
char* mpp_get_curr_pkt( t_mpegts_parser *mpp );

#endif /* T_MPEGTS_PARSER_H_ */

