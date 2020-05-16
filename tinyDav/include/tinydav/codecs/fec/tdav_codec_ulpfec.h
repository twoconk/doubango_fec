/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*
* Contact: Mamadou Diop <diopmamadou(at)doubango[dot]org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file tdav_codec_ulpfec.h
 * @brief Forward Error Correction (FEC) implementation as per RFC 5109
 */
#ifndef TINYDAV_CODEC_ULPFEC_H
#define TINYDAV_CODEC_ULPFEC_H

#include "tinydav_config.h"

#include "tinymedia/tmedia_codec.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"

TDAV_BEGIN_DECLS

struct tdav_codec_ulpfec_s;
struct trtp_rtp_packet_s;

//
//	FEC LEVEL
//
typedef struct tdav_fec_level_s
{
	TSK_DECLARE_OBJECT;

	struct{ // 7.4. FEC Level Header for FEC Packets
		uint16_t length;
		uint64_t mask;
		tsk_size_t mask_size; // in bits
	} hdr;
	struct{
		uint8_t* ptr;
		tsk_size_t size;
	}payload;
}tdav_fec_level_t;
typedef tsk_list_t tdav_fec_levels_L_t;

//
//	FEC PACKET
//
typedef struct tdav_fec_pkt_s
{ 
	TSK_DECLARE_OBJECT;

	struct{ // RFC 5109 - 7.3. FEC Header for FEC Packets
		unsigned E : 1;
		unsigned L : 1;
		unsigned P : 1;
		unsigned X : 1;
		unsigned CC : 4;
		unsigned M : 1;
		unsigned PT : 7;
		struct{
			uint16_t value;
			unsigned set : 1;
		}SN_base;
		uint32_t TS;
		uint16_t length;
	}hdr;

	tdav_fec_levels_L_t* levels;
}
tdav_fec_pkt_t;

int tdav_codec_ulpfec_enc_reset(struct tdav_codec_ulpfec_s* self);
int tdav_codec_ulpfec_enc_protect(struct tdav_codec_ulpfec_s* self, const struct trtp_rtp_packet_s* rtp_packet);
tsk_size_t tdav_codec_ulpfec_enc_serialize(const struct tdav_codec_ulpfec_s* self, void** out_data, tsk_size_t* out_max_size);
tdav_fec_pkt_t* tdav_codec_ulpfec_dec_deserialize(const trtp_rtp_packet_t* rtp_packet);
trtp_rtp_packet_t* tdav_code_ulpfec_recover_packet(tdav_fec_pkt_t *fec_pkt, tsk_list_t *list_recv_pkt, int target_seq);

TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_ulpfec_plugin_def_t;

TDAV_END_DECLS

#endif /* TINYDAV_CODEC_ULPFEC_H */