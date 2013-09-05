/*
 * t_mpegts_parser.cpp
 *
 *  Created on: Apr 17, 2012
 *      Author: yusuf
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "logjam.h"

#include "ts_parser.h"


t_mpegts_parser *mpp_new ( const char *file )
{
	t_mpegts_parser *mpp = (t_mpegts_parser*)calloc ( 1, sizeof(t_mpegts_parser) );
	memset ( mpp, 0, sizeof(*mpp) );

	mpp->file = fopen( file, "rb" );
	if( !mpp->file )
	{
		LOGE( "mpeg-ts file can not open\n" );
		return NULL;
	}

	return mpp;
}

void mpp_delete ( t_mpegts_parser *mpp )
{
	fclose( mpp->file );
	free ( mpp );
}

int decode_tsheader( t_mpegts_parser *mpp )
{
	mpp->curr_tsheader.transport_error_indicator = (mpp->curr_pkt[0] & 0x80) >> 7;
	mpp->curr_tsheader.payload_start_indicator = (mpp->curr_pkt[0] & 0x40) >> 6;
	mpp->curr_tsheader.transport_priority = (mpp->curr_pkt[0] & 0x20) >> 5;
	mpp->curr_tsheader.PID = ((mpp->curr_pkt[0] & 0x1F) << 8) | mpp->curr_pkt[1];
	mpp->curr_tsheader.transport_scrambling_control = (mpp->curr_pkt[2] & 0xC0);
	mpp->curr_tsheader.adaption_field_control = (mpp->curr_pkt[2] & 0x30) >> 4;
	mpp->curr_tsheader.continuity_counter = (mpp->curr_pkt[2] & 0xF);

	mpp->read_pos = 3;

	return 1;
}

int decode_adaptation_field( t_mpegts_parser *mpp )
{
	int ind = mpp->read_pos;
	mpp->adaptation_field.field_lenght = mpp->curr_pkt[ind];
	ind++;

	if( mpp->adaptation_field.field_lenght > 0 )
	{
		mpp->adaptation_field.af_ext_flag = ( mpp->curr_pkt[ind] & 0x01 );
		mpp->adaptation_field.transport_private_data_flag = ( mpp->curr_pkt[ind] & 0x02 ) >> 1;
		mpp->adaptation_field.splicing_point_flag = ( mpp->curr_pkt[ind] & 0x04 ) >> 2;
		mpp->adaptation_field.opcr_flag = ( mpp->curr_pkt[ind] & 0x08 ) >> 3;
		mpp->adaptation_field.pcr_flag = ( mpp->curr_pkt[ind] & 0x10 ) >> 4;
		mpp->adaptation_field.es_priority_indicator = ( mpp->curr_pkt[ind] & 0x20 ) >> 5;
		mpp->adaptation_field.random_access_indicator = ( mpp->curr_pkt[ind] & 0x40 ) >> 6;
		mpp->adaptation_field.discontinuity_indicator = ( mpp->curr_pkt[ind] & 0x80 ) >> 7;
		ind++;

		if( mpp->adaptation_field.pcr_flag )
		{
			mpp->adaptation_field.pcr = ( (long int)mpp->curr_pkt[ind] ) << 25;
			mpp->adaptation_field.pcr += ( (long int) mpp->curr_pkt[ind+1] ) << 17;
			mpp->adaptation_field.pcr += mpp->curr_pkt[ind+2] << 9;
			mpp->adaptation_field.pcr += mpp->curr_pkt[ind+3] << 1;
			mpp->adaptation_field.pcr += ( mpp->curr_pkt[ind+4] & 0x80 ) >> 7;
			mpp->adaptation_field.pcr *= 300;
			mpp->adaptation_field.pcr += ( mpp->curr_pkt[ind+4] & 0x01 ) << 8;
			mpp->adaptation_field.pcr += mpp->curr_pkt[ind+5];
			ind += 6;
		}

		if( mpp->adaptation_field.opcr_flag )
		{
			mpp->adaptation_field.opcr = ( (long int)mpp->curr_pkt[ind] ) << 25;
			mpp->adaptation_field.opcr += ( (long int) mpp->curr_pkt[ind+1] ) << 17;
			mpp->adaptation_field.opcr += mpp->curr_pkt[ind+2] << 9;
			mpp->adaptation_field.opcr += mpp->curr_pkt[ind+3] << 1;
			mpp->adaptation_field.opcr += ( mpp->curr_pkt[ind+4] & 0x80 ) >> 7;
			mpp->adaptation_field.opcr *= 300;
			mpp->adaptation_field.opcr += ( mpp->curr_pkt[ind+4] & 0x01 ) << 8;
			mpp->adaptation_field.opcr += mpp->curr_pkt[ind+5];
			ind += 6;
		}

		if( mpp->adaptation_field.splicing_point_flag )
		{
			mpp->adaptation_field.splice_countdown = mpp->curr_pkt[ind];
			ind++;
		}

		if( mpp->adaptation_field.transport_private_data_flag )
		{

		}

		if( mpp->adaptation_field.af_ext_flag )
		{

		}

	}

	mpp->read_pos += mpp->adaptation_field.field_lenght + 1;
	return 1;
}

void print_af_info( t_mpegts_parser *mpp )
{
	LOGD("AF field_lenght : %d\n",mpp->adaptation_field.field_lenght );
	LOGD("AF discontinuity_indicator : %d\n",mpp->adaptation_field.discontinuity_indicator );
	LOGD("AF random_access_indicator : %d\n",mpp->adaptation_field.random_access_indicator );
	LOGD("AF es_priority_indicator : %d\n",mpp->adaptation_field.es_priority_indicator );
	LOGD("AF pcr_flag : %d\n",mpp->adaptation_field.pcr_flag );
	LOGD("AF opcr_flag : %d\n",mpp->adaptation_field.opcr_flag );
	LOGD("AF splicing_point_flag : %d\n",mpp->adaptation_field.splicing_point_flag );
	LOGD("AF transport_private_data_flag : %d\n",mpp->adaptation_field.transport_private_data_flag );
	LOGD("AF af_ext_flag : %d\n",mpp->adaptation_field.af_ext_flag );
	if( mpp->adaptation_field.pcr_flag )
	{
		LOGD("AF pcr : %ld\n",mpp->adaptation_field.pcr );
	}
}


int decode_pat( t_mpegts_parser *mpp )
{
	unsigned int i = 0;
	int ind = mpp->read_pos;
	int pointer_field = mpp->curr_pkt[ind];
	ind += pointer_field + 1;
	mpp->pat.table_id = mpp->curr_pkt[ind];
	if( mpp->pat.table_id != 0 )
	{
		LOGE( "Not a valid mpeg-ts pat\n" );
		return 0;
	}
	mpp->pat.section_syntax_indicator = (mpp->curr_pkt[ind+1] & 0x80) >> 7;
	if( mpp->pat.section_syntax_indicator != 1 )
	{
		LOGE( "Unsupported table format\n" );
		//return 0;
	}
	mpp->pat.section_lenght = ((mpp->curr_pkt[ind+1] & 0x0F) << 8) | mpp->curr_pkt[ind+2];
	mpp->pat.ts_id = ((mpp->curr_pkt[ind+3]) << 8) | mpp->curr_pkt[ind+4];
	mpp->pat.version_number = (mpp->curr_pkt[ind+5] & 0x3E) >> 1;
	mpp->pat.current_next_indicator = (mpp->curr_pkt[ind+5] & 0x01);
	mpp->pat.section_number = mpp->curr_pkt[ind+6];
	mpp->pat.last_section_number = mpp->curr_pkt[ind+7];

	unsigned int prog_count = ( mpp->pat.section_lenght - 9 ) / 4;

	mpp->pat.prog_count = prog_count;

	for( i = 0; i < mpp->pat.prog_count; i++ )
	{
		mpp->pat.prog_infos[i].number = ( mpp->curr_pkt[ind+8+i] << 8 ) | mpp->curr_pkt[ind+9+i];
		mpp->pat.prog_infos[i].pid = ( ( mpp->curr_pkt[ind+10+i] & 0x1F ) << 8 ) | mpp->curr_pkt[ind+11+i];
	}

	mpp->read_pos += mpp->pat.section_lenght + pointer_field + 4;

	return 1;
}

void print_pat_info( t_mpegts_parser *mpp )
{
	unsigned int i = 0;
	LOGD("PAT table id : %d\n",mpp->pat.table_id );
	LOGD("PAT section_syntax_indicator : %d\n",mpp->pat.section_syntax_indicator );
	LOGD("PAT section_lenght : %d\n",mpp->pat.section_lenght );
	LOGD("PAT ts_id : %d\n",mpp->pat.ts_id );
	LOGD("PAT version_number : %d\n",mpp->pat.version_number );
	LOGD("PAT current_next_indicator : %d\n",mpp->pat.current_next_indicator );
	LOGD("PAT section_number : %d\n",mpp->pat.section_number );
	LOGD("PAT last_section_number : %d\n",mpp->pat.last_section_number );
	for( i = 0; i < mpp->pat.prog_count; i++ )
	{
		LOGD( "PAT program_number and PID: %d  %d\n", mpp->pat.prog_infos[i].number, mpp->pat.prog_infos[i].pid );
	}
}


int decode_pmt( t_mpegts_parser *mpp )
{
	int ind = mpp->read_pos;
	int pointer_field = mpp->curr_pkt[ind];
	ind += pointer_field + 1;
	mpp->pmt.table_id = mpp->curr_pkt[ind];
	if( mpp->pmt.table_id != 2 )
	{
		LOGE( "Not a valid mpeg-ts pmt\n" );
		return 0;
	}
	mpp->pmt.section_syntax_indicator = (mpp->curr_pkt[ind+1] & 0x80) >> 7;
	if( mpp->pat.section_syntax_indicator != 1 )
	{
		LOGE( "Unsupported table format\n" );
		//return 0;
	}
	mpp->pmt.section_lenght = ((mpp->curr_pkt[ind+1] & 0x0F) << 8) | mpp->curr_pkt[ind+2];
	mpp->pmt.prog_num = ((mpp->curr_pkt[ind+3]) << 8) | mpp->curr_pkt[ind+4];
	mpp->pmt.version_number = (mpp->curr_pkt[ind+5] & 0x3E) >> 1;
	mpp->pmt.current_next_indicator = (mpp->curr_pkt[ind+5] & 0x01);
	mpp->pmt.section_number = mpp->curr_pkt[ind+6];
	mpp->pmt.last_section_number = mpp->curr_pkt[ind+7];
	mpp->pmt.pcr_pid = ( ( mpp->curr_pkt[ind+8] & 0x1F ) << 8 ) | mpp->curr_pkt[ind+9];
	mpp->pmt.prog_info_lenght = ( ( mpp->curr_pkt[ind+10] & 0x0F ) << 8 ) | mpp->curr_pkt[ind+11];
	ind += mpp->pmt.prog_info_lenght;

	mpp->pmt.es_count = 0;
	int total_bytes = 0;
	while( (mpp->pmt.section_lenght - 13 - total_bytes -mpp->pmt.prog_info_lenght) > 0 )
	{
		mpp->pmt.es_infos[mpp->pmt.es_count].stream_type = mpp->curr_pkt[ind+12];
		mpp->pmt.es_infos[mpp->pmt.es_count].pid = ( ( mpp->curr_pkt[ind+13] & 0x1F ) << 8 ) | mpp->curr_pkt[ind+14];
		int es_info_lenght = ( ( ( mpp->curr_pkt[ind+15] & 0x0F ) << 8 ) | mpp->curr_pkt[ind+16] );
		ind += es_info_lenght + 5;
		total_bytes += es_info_lenght + 5;
		mpp->pmt.es_count++;
	}

	mpp->read_pos += mpp->pmt.section_lenght + pointer_field + 4;

	return 1;
}

void print_pmt_info( t_mpegts_parser *mpp )
{
	unsigned int i = 0;
	LOGD("PMT table id : %d\n",mpp->pmt.table_id );
	LOGD("PMT section_syntax_indicator : %d\n",mpp->pmt.section_syntax_indicator );
	LOGD("PMT section_lenght : %d\n",mpp->pmt.section_lenght );
	LOGD("PMT prog_num : %d\n",mpp->pmt.prog_num );
	LOGD("PMT version_number : %d\n",mpp->pmt.version_number );
	LOGD("PMT current_next_indicator : %d\n",mpp->pmt.current_next_indicator );
	LOGD("PMT section_number : %d\n",mpp->pmt.section_number );
	LOGD("PMT last_section_number : %d\n",mpp->pmt.last_section_number );
	LOGD("PMT pcr_pid : %d\n",mpp->pmt.pcr_pid );
	LOGD("PMT prog_info_lenght : %d\n",mpp->pmt.prog_info_lenght );
	LOGD("Stream Count %d\n", mpp->pmt.es_count);
	for( i = 0; i < mpp->pmt.es_count; i++ )
	{
		LOGD( "PMT ES type and PID: %d  %d\n", mpp->pmt.es_infos[i].stream_type, mpp->pmt.es_infos[i].pid );
	}
}

int decode_pes( t_mpegts_parser *mpp )
{
	int ind = mpp->read_pos;

	mpp->pes.start_code = ( mpp->curr_pkt[ind] << 16 ) | ( mpp->curr_pkt[ind+1] << 8 ) | ( mpp->curr_pkt[ind+2] );
	mpp->pes.id = mpp->curr_pkt[ind+3];
	mpp->pes.packet_lenght = ( mpp->curr_pkt[ind+4] << 8 ) | mpp->curr_pkt[ind+5];

	if( mpp->pes.id != 0xBC && mpp->pes.id != 0xBE && mpp->pes.id != 0xBF &&
		mpp->pes.id != 0xF0 && mpp->pes.id != 0xF1 && mpp->pes.id != 0xFF &&
		mpp->pes.id != 0xF2 && mpp->pes.id != 0xF8 )
	{
		mpp->pes.scr_control = ( mpp->curr_pkt[ind+6] & 0x30 ) >> 4;
		mpp->pes.priority = ( mpp->curr_pkt[ind+6] & 0x08 ) >> 3;
		mpp->pes.data_alignment = ( mpp->curr_pkt[ind+6] & 0x04 ) >> 2;
		mpp->pes.copyright = ( mpp->curr_pkt[ind+6] & 0x02 ) >> 1;
		mpp->pes.original = ( mpp->curr_pkt[ind+6] & 0x01 );
		mpp->pes.pts_dts_flag = ( mpp->curr_pkt[ind+7] & 0xC0 ) >> 6;
		mpp->pes.escr_flag = ( mpp->curr_pkt[ind+7] & 0x20 ) >> 5;
		mpp->pes.es_rate_flag = ( mpp->curr_pkt[ind+7] & 0x10 ) >> 4;
		mpp->pes.dsm_trick_mode_flag = ( mpp->curr_pkt[ind+7] & 0x08 ) >> 3;
		mpp->pes.add_copy_info_flag = ( mpp->curr_pkt[ind+7] & 0x04 ) >> 2;
		mpp->pes.crc_flag = ( mpp->curr_pkt[ind+7] & 0x02 ) >> 1;
		mpp->pes.extension_flag = ( mpp->curr_pkt[ind+7] & 0x01 );
		mpp->pes.header_lenght = mpp->curr_pkt[ind+8];
		ind += 9;

		if( mpp->pes.pts_dts_flag > 1 )
		{
			mpp->pes.pts = ( (long int)( mpp->curr_pkt[ind] & 0x0E ) ) << 30;
			mpp->pes.pts += ( (long int)mpp->curr_pkt[ind+1] ) << 22;
			mpp->pes.pts += ( mpp->curr_pkt[ind+2] & 0xFE ) << 14;
			mpp->pes.pts += mpp->curr_pkt[ind+3] << 7;
			mpp->pes.pts += ( mpp->curr_pkt[ind+3] & 0xFE ) >> 1;
			ind += 5;
		}

		if( mpp->pes.pts_dts_flag == 3 )
		{
			mpp->pes.dts = ( (long int)( mpp->curr_pkt[ind] & 0x0E ) ) << 30;
			mpp->pes.dts += ( (long int)mpp->curr_pkt[ind+1] ) << 22;
			mpp->pes.dts += ( mpp->curr_pkt[ind+2] & 0xFE ) << 14;
			mpp->pes.dts += mpp->curr_pkt[ind+3] << 7;
			mpp->pes.dts += ( mpp->curr_pkt[ind+3] & 0xFE ) >> 1;
			ind += 5;
		}

		if( mpp->pes.escr_flag )
		{

		}

		if( mpp->pes.es_rate_flag )
		{

		}

		if( mpp->pes.dsm_trick_mode_flag )
		{

		}

		if( mpp->pes.add_copy_info_flag )
		{

		}

		if( mpp->pes.crc_flag )
		{

		}

		if( mpp->pes.extension_flag )
		{

		}
	}

	return 1;
}

void print_pes_info( t_mpegts_parser *mpp )
{
	LOGD("PES start_code : %d\n",mpp->pes.start_code );
	LOGD("PES id : %d\n",mpp->pes.id );
	LOGD("PES packet_lenght : %d\n",mpp->pes.packet_lenght );
	if( mpp->pes.id != 0xBC && mpp->pes.id != 0xBE && mpp->pes.id != 0xBF &&
			mpp->pes.id != 0xF0 && mpp->pes.id != 0xF1 && mpp->pes.id != 0xFF &&
			mpp->pes.id != 0xF2 && mpp->pes.id != 0xF8 )
	{
		LOGD("PES scr_control : %d\n",mpp->pes.scr_control );
		LOGD("PES priority : %d\n",mpp->pes.priority );
		LOGD("PES data_alignment : %d\n",mpp->pes.data_alignment );
		LOGD("PES copyright : %d\n",mpp->pes.copyright );
		LOGD("PES original : %d\n",mpp->pes.original );
		LOGD("PES pts_dts_flag : %d\n",mpp->pes.pts_dts_flag );
		LOGD("PES escr_flag : %d\n",mpp->pes.escr_flag );
		LOGD("PES es_rate_flag : %d\n",mpp->pes.es_rate_flag );
		LOGD("PES dsm_trick_mode_flag : %d\n",mpp->pes.dsm_trick_mode_flag );
		LOGD("PES add_copy_info_flag : %d\n",mpp->pes.add_copy_info_flag );
		LOGD("PES crc_flag : %d\n",mpp->pes.crc_flag );
		LOGD("PES extension_flag : %d\n",mpp->pes.extension_flag );
		LOGD("PES header_lenght : %d\n",mpp->pes.header_lenght );
		LOGD("PES pts : %f\n",mpp->pes.pts / 90000.0 );
		LOGD("PES dts : %f\n",mpp->pes.dts / 90000.0 );
	}
}

void print_packet_info( t_mpegts_parser *mpp )
{
	LOGD("Header transport_error_indicator : %d\n",mpp->curr_tsheader.transport_error_indicator );
	LOGD("Header payload start indicator : %d\n",mpp->curr_tsheader.payload_start_indicator );
	LOGD("Header transport_priority : %d\n",mpp->curr_tsheader.transport_priority );
	LOGD("Header PID : %d\n",mpp->curr_tsheader.PID );
	LOGD("Header transport_scrambling_control : %d\n",mpp->curr_tsheader.transport_scrambling_control );
	LOGD("Header adaption_field_control : %d\n",mpp->curr_tsheader.adaption_field_control );
	LOGD("Header continuity_counter : %d\n",mpp->curr_tsheader.continuity_counter );

}

void mpp_add_es_cb( t_mpegts_parser *mpp, t_mpegts_es_cb *cb, void *arg, unsigned int es_pid  )
{
	mpp->es_cb_funcs[mpp->es_cb_count] = cb;
	mpp->es_cb_pids[mpp->es_cb_count] = es_pid;
	mpp->es_cb_args[mpp->es_cb_count] = arg;
	mpp->es_cb_count++;
}

void mpp_set_pat_cb( t_mpegts_parser *mpp, t_mpegts_pat_cb *cb, void *arg  )
{
	mpp->pat_cb_func = cb;
	mpp->pat_cb_arg = arg;
}

void mpp_set_pmt_cb( t_mpegts_parser *mpp, t_mpegts_pmt_cb *cb, void *arg, unsigned int prog_pid   )
{
	mpp->pmt_cb_func = cb;
	mpp->pmt_cb_arg = arg;
	mpp->pmt_pid = prog_pid;
}

void mpp_set_pcr_cb( t_mpegts_parser *mpp, t_mpegts_pcr_cb *cb, void *arg )
{
	mpp->pcr_cb_func = cb;
	mpp->pcr_cb_arg = arg;
}

void mpp_clear_pat_cb( t_mpegts_parser *mpp )
{
	mpp->pat_cb_func = NULL;
	mpp->pat_cb_arg = NULL;
}

void mpp_clear_pmt_cb( t_mpegts_parser *mpp )
{
	mpp->pmt_cb_func = NULL;
	mpp->pmt_cb_arg = NULL;
}

void mpp_clear_pcr_cb( t_mpegts_parser *mpp )
{
	mpp->pcr_cb_func = NULL;
	mpp->pcr_cb_arg = NULL;
}

void mpp_clear_es_cb( t_mpegts_parser *mpp )
{
	mpp->es_cb_count = 0;
}

void mpp_remove_es_cb( t_mpegts_parser *mpp, unsigned int es_pid )
{
	int i = 0;
	int j = 0;
	for( i = 0; i < mpp->es_cb_count; i++ )
	{
		if( es_pid == mpp->es_cb_pids[i] )
		{
			mpp->es_cb_count--;
			for( j = i; j < mpp->es_cb_count; j++ )
			{
				mpp->es_cb_funcs[j] = mpp->es_cb_funcs[j+1];
				mpp->es_cb_pids[j] = mpp->es_cb_pids[j+1];
				mpp->es_cb_args[j] = mpp->es_cb_args[j+1];
			}
			break;
		}
	}
}

char* mpp_get_curr_pkt( t_mpegts_parser *mpp )
{
	return (char*)mpp->curr_pkt;
}

int pidc = 0;
int pids[100];

int mpp_parse_next_pkt( t_mpegts_parser *mpp )
{
	int i = 0;
	const char sync_byte = 0x47;

	if( !mpp || !mpp->file )
	{
		return 0;
	}

	char c = 0;
	int actual = 0;
	int read_cnt = 0;

	do
	{
		actual = fread( &c, 1, 1, mpp->file );
		if( actual != 1 )
		{
			return 0;
		}
		read_cnt++;
	}while( c != sync_byte );

	if( read_cnt > 1 )
	{
		LOGE( " Corrupted ts packet. Sync byte found %d bytes after\n", read_cnt-1 );
	}

	actual = fread( mpp->curr_pkt, 1, TS_PACKET_LEN, mpp->file );
	if( actual != TS_PACKET_LEN )
	{
		LOGE( "File should not end here \n" );
		return 0;
	}

	decode_tsheader( mpp );

	if( mpp->curr_tsheader.adaption_field_control == 2 || mpp->curr_tsheader.adaption_field_control == 3 )
	{
		decode_adaptation_field( mpp );
		if( mpp->adaptation_field.pcr_flag && mpp->pcr_cb_func )
		{
			mpp->pcr_cb_func( mpp->pcr_cb_arg, mpp->adaptation_field.pcr );
		}
		if( mpp->adaptation_field.pcr_flag /*&& mpp->adaptation_field.pcr < 27000000*13*/ )
		{
			//LOGD( "PCR Found : %f\n", (mpp->adaptation_field.pcr/27000000.0) );
		}
	}

	int found = 0;
	for( i = 0; i < pidc; i++ )
	{
		if( mpp->curr_tsheader.PID == pids[i] )
		{
			found = 1;
		}
	}
	if( found == 0 )
	{
		pids[pidc] = mpp->curr_tsheader.PID;
		pidc++;
		LOGD("PID found %d\n", mpp->curr_tsheader.PID);
	}

	if( mpp->curr_tsheader.PID == 0 && mpp->pat_cb_func )
	{
		LOGD("\nPAT found\n");

		decode_pat( mpp );
		print_packet_info( mpp );
		print_pat_info( mpp );

		mpp->pat_cb_func( mpp->pat_cb_arg, mpp->pat.prog_count, mpp->pat.prog_infos );
	}

	if( mpp->pmt_cb_func && mpp->curr_tsheader.PID == mpp->pmt_pid )
	{
		LOGD("\nPMT found\n");

		decode_pmt( mpp );
		print_packet_info( mpp );
		print_pmt_info( mpp );

		mpp->pmt_cb_func( mpp->pmt_cb_arg, mpp->pmt.es_count, mpp->pmt.es_infos  );
	}

	for( i = 0; i < mpp->es_cb_count; i++ )
	{
		if( mpp->curr_tsheader.PID == mpp->es_cb_pids[i] )
		{
			//LOGD("\nES found\n");
			if( mpp->curr_tsheader.payload_start_indicator )
			{
				//print_packet_info( mpp );
				//print_af_info( mpp );
				decode_pes( mpp );
				//print_pes_info( mpp );
				/*if( mpp->pes.pts_dts_flag > 1 && mpp->pes.pts < 1200000 )
				{
					LOGD("PTS found : %ld\n", mpp->pes.pts);
				}*/
			}
			mpp->es_cb_funcs[i]( mpp->es_cb_args[i], mpp->es_cb_pids[i], mpp->curr_tsheader.payload_start_indicator,
								 mpp->pes.pts, (const char*)mpp->curr_pkt );
		}
	}


	return 1;
}





