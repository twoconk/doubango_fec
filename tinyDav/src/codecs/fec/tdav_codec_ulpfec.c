/*
* Copyright (C) 2012-2015 Doubango Telecom <http://www.doubango.org>
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

/**@file tdav_codec_ulpfec.c
 * @brief Forward Error Correction (FEC) implementation as per RFC 5109
 */
#include "tinydav/codecs/fec/tdav_codec_ulpfec.h"


#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_debug.h"

#define TDAV_FEC_PKT_HDR_SIZE	10

typedef struct tdav_codec_ulpfec_s
{
	TMEDIA_DECLARE_CODEC_VIDEO;

	struct{
		struct tdav_fec_pkt_s* pkt;
	} encoder;
}
tdav_codec_ulpfec_t;

static tsk_object_t* tdav_fec_level_ctor(tsk_object_t * self, va_list * app)
{
	tdav_fec_level_t *level = self;
	if (level){
		level->hdr.mask_size = 16; // L=0
	}
	return self;
}
static tsk_object_t* tdav_fec_level_dtor(tsk_object_t * self)
{
	tdav_fec_level_t *level = self;
	if (level){
		TSK_FREE(level->payload.ptr);
	}

	return self;
}
static const tsk_object_def_t tdav_fec_level_def_s =
{
	sizeof(tdav_fec_level_t),
	tdav_fec_level_ctor,
	tdav_fec_level_dtor,
	tsk_null,
};
const tsk_object_def_t *tdav_fec_level_def_t = &tdav_fec_level_def_s;


static tsk_object_t* tdav_fec_pkt_ctor(tsk_object_t * self, va_list * app)
{
	tdav_fec_pkt_t *pkt = self;
	if (pkt){
		if (!(pkt->levels = tsk_list_create())){
			TSK_DEBUG_ERROR("Failed to create levels");
			return tsk_null;
		}
	}
	return self;
}
static tsk_object_t* tdav_fec_pkt_dtor(tsk_object_t * self)
{
	tdav_fec_pkt_t *pkt = self;
	if (pkt){
		TSK_OBJECT_SAFE_FREE(pkt->levels);
	}

	return self;
}
static int tdav_fec_pkt_cmp(const tsk_object_t *_p1, const tsk_object_t *_p2)
{
	const tdav_fec_pkt_t *p1 = _p1;
	const tdav_fec_pkt_t *p2 = _p2;

	if (p1 && p2){
		return (int)(p1->hdr.SN_base.value - p2->hdr.SN_base.value);
	}
	else if (!p1 && !p2) return 0;
	else return -1;
}
static const tsk_object_def_t tdav_fec_pkt_def_s =
{
	sizeof(tdav_fec_pkt_t),
	tdav_fec_pkt_ctor,
	tdav_fec_pkt_dtor,
	tdav_fec_pkt_cmp,
};
const tsk_object_def_t *tdav_fec_pkt_def_t = &tdav_fec_pkt_def_s;


tsk_size_t tdav_codec_ulpfec_guess_serialbuff_size(const tdav_codec_ulpfec_t* self)
{
	tsk_size_t size = TDAV_FEC_PKT_HDR_SIZE;
	tsk_list_item_t *item;
	tdav_fec_level_t* level;

	if (!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return 0;
	}

	tsk_list_foreach(item, self->encoder.pkt->levels){
		if (!(level = item->data)){
			continue;
		}
		size += 2 /* Protection length */ + (level->hdr.mask_size >> 3) + level->hdr.length;
	}

	return size;
}

int tdav_codec_ulpfec_enc_reset(tdav_codec_ulpfec_t* self)
{
	tsk_list_item_t *item;
	tdav_fec_level_t* level;

	if (!self || !self->encoder.pkt){
		TSK_DEBUG_ERROR("invalid parameter");
		return -1;
	}

	// reset packet
	memset(&self->encoder.pkt->hdr, 0, sizeof(self->encoder.pkt->hdr));

	// reset levels
	tsk_list_foreach(item, self->encoder.pkt->levels){
		if ((level = item->data)){
			memset(&level->hdr, 0, sizeof(level->hdr));
			if (level->payload.ptr){
				memset(level->payload.ptr, 0, level->payload.size);
			}
		}
	}
	return 0;
}

int tdav_codec_ulpfec_enc_protect(tdav_codec_ulpfec_t* self, const trtp_rtp_packet_t* rtp_packet)
{
	if (!self || !self->encoder.pkt || !rtp_packet || !rtp_packet->header){
		TSK_DEBUG_ERROR("invalid parameter");
		return -1;
	}

	// Packet
	self->encoder.pkt->hdr.P ^= rtp_packet->header->padding;
	self->encoder.pkt->hdr.X ^= rtp_packet->header->extension;
	self->encoder.pkt->hdr.CC = rtp_packet->header->csrc_count;
	self->encoder.pkt->hdr.M = 0;//rtp_packet->header->marker;
	self->encoder.pkt->hdr.PT = rtp_packet->header->payload_type;//^= rtp_packet->header->payload_type;
	/*
	if (!self->encoder.pkt->hdr.SN_base.set){
		self->encoder.pkt->hdr.SN_base.value = rtp_packet->header->seq_num;
		self->encoder.pkt->hdr.SN_base.set = 1;
	}
	else{*/
		self->encoder.pkt->hdr.SN_base.value ^= rtp_packet->header->seq_num;//TSK_MAX(self->encoder.pkt->hdr.SN_base.value, rtp_packet->header->seq_num);
	//}
	//self->encoder.pkt->hdr.TS ^= rtp_packet->header->timestamp;
	self->encoder.pkt->hdr.TS = rtp_packet->header->timestamp;
	self->encoder.pkt->hdr.length ^= rtp_packet->payload.size;//(trtp_rtp_packet_guess_serialbuff_size(rtp_packet) - TRTP_RTP_HEADER_MIN_SIZE);

	// Level
	// For now, always single-level protection
	{
		tdav_fec_level_t* level0 = TSK_LIST_FIRST_DATA(self->encoder.pkt->levels);
		const uint8_t* rtp_payload = (const uint8_t*)(rtp_packet->payload.data_const ? rtp_packet->payload.data_const : rtp_packet->payload.data);
		tsk_size_t i;
		if (!level0){
			tdav_fec_level_t* _level0;
			if (!(_level0 = tsk_object_new(tdav_fec_level_def_t))){
				TSK_DEBUG_ERROR("Failed to create level");
				return -2;
			}
			level0 = _level0;
			tsk_list_push_back_data(self->encoder.pkt->levels, (void**)&_level0);
		}
		if (level0->payload.size < rtp_packet->payload.size){
			if (!(level0->payload.ptr = tsk_realloc(level0->payload.ptr, rtp_packet->payload.size))){
				TSK_DEBUG_ERROR("Failed to realloc size %d", rtp_packet->payload.size);
				level0->payload.size = 0;
				return -3;
			}
			level0->payload.size = rtp_packet->payload.size;
		}
		for (i = 0; i < rtp_packet->payload.size; ++i){
			level0->payload.ptr[i] ^= rtp_payload[i];
		}
		level0->hdr.mask_size = self->encoder.pkt->hdr.L ? 48 : 16;
		level0->hdr.mask |= (uint64_t)((uint64_t)1 << (level0->hdr.mask_size - (rtp_packet->header->seq_num - self->encoder.pkt->hdr.SN_base.value)));
		level0->hdr.length = (uint16_t)(TSK_MAX(level0->hdr.length, rtp_packet->payload.size));
	}

	return 0;
}


trtp_rtp_packet_t* tdav_code_ulpfec_recover_packet(tdav_fec_pkt_t *fec_pkt, tsk_list_t *list_recv_pkt, int target_seq){
    if (!fec_pkt || !list_recv_pkt || !fec_pkt->levels){
        TSK_DEBUG_ERROR("fec recover invalid parameter");
        return NULL;
    }

    trtp_rtp_packet_t *ret_pkt = tsk_null;
    tsk_list_item_t *item;
    int pkt_length = fec_pkt->hdr.length;
    int pkt_seq = fec_pkt->hdr.SN_base.value;
    int end_seq = 0;
    trtp_rtp_header_t *header = tsk_null;
    int i = 0;
    
    tsk_list_foreach(item, list_recv_pkt){
        trtp_rtp_packet_t *data = item->data;
        if (data == tsk_null){
            continue;
        }
        pkt_seq ^= data->header->seq_num;
        pkt_length  ^= data->payload.size;

        end_seq = data->header->seq_num;
    }

    TSK_DEBUG_INFO("fec recover, find pkt_seq:%d, target_seq:%d, pkt_length:%d", pkt_seq, target_seq, pkt_length);
    if (pkt_seq != target_seq || pkt_length == 0){
        TSK_DEBUG_ERROR("fec recover invalid parameter");
        return NULL;
    }
    
    if(!(header = trtp_rtp_header_create_null())){
    	TSK_DEBUG_ERROR("fec failed to create new RTP header");
    	return tsk_null;
    }

    /* version (2bits) */
    header->version = 2;

    /* Marker (1bit) */
    header->marker = end_seq > target_seq? tsk_false:tsk_true;
    /* Payload Type (7bits) */
    header->payload_type = 98;

    /* Sequence Number (16bits) */
    header->seq_num = target_seq;

    /* timestamp (32bits) */
    header->timestamp = fec_pkt->hdr.TS;

    /* create the packet */
    if(!(ret_pkt = trtp_rtp_packet_create_null())){
        TSK_DEBUG_ERROR("fec failed to create new RTP packet");
        TSK_OBJECT_SAFE_FREE(header);
        return tsk_null;
    }
    /* set the header */
    ret_pkt->header = header;

    ret_pkt->payload.size = pkt_length;

    ret_pkt->payload.data = tsk_malloc(pkt_length);
    if (ret_pkt->payload.data == tsk_null){
        TSK_DEBUG_ERROR("fec failed to create new RTP payload");
        TSK_OBJECT_SAFE_FREE(ret_pkt);
        return tsk_null;
    }

    tdav_fec_level_t* level0 = TSK_LIST_FIRST_DATA(fec_pkt->levels);
    if (level0 == tsk_null){
        TSK_DEBUG_ERROR("fec level0 is null");
        TSK_OBJECT_SAFE_FREE(ret_pkt);
        return tsk_null;
    }

    //first copy data
    memcpy(ret_pkt->payload.data, level0->payload.ptr, pkt_length);
    tsk_list_foreach(item, list_recv_pkt){
        trtp_rtp_packet_t *data = item->data;
        if (data == tsk_null){
            continue;
        }
        uint8_t* rtp_payload = (uint8_t*)data->payload.data;
        uint8_t* target_payload = (uint8_t*)ret_pkt->payload.data;
        //xor recover data
        for (; i < pkt_length; ++i){
            target_payload[i] ^= rtp_payload[i];
        }
    } 
    TSK_DEBUG_INFO("fec recover success");

    return ret_pkt;
}

tdav_fec_pkt_t* tdav_codec_ulpfec_dec_deserialize(const trtp_rtp_packet_t* rtp_packet){
    uint8_t* pdata;
    tdav_fec_level_t* level0;
    tdav_fec_level_t* temp;
    int protect_length = 0;
    tdav_fec_pkt_t* pkt;
    if (!rtp_packet || !rtp_packet->header){
        TSK_DEBUG_ERROR("invalid parameter");
        return NULL;
    }

    pdata = rtp_packet->payload.data ? rtp_packet->payload.data : rtp_packet->payload.data_const;
    if (pdata == tsk_null){
        TSK_DEBUG_ERROR("tdav_codec_ulpfec_dec_deserialize pdata == null");
        return NULL;
    }
    pkt = tsk_object_new(tdav_fec_pkt_def_t);
    if (pkt == tsk_null){
        TSK_DEBUG_ERROR("tdav_codec_ulpfec_dec_deserialize pkt == null");
        return NULL;
    }
    if (!(pkt->levels = tsk_list_create())){
    	TSK_DEBUG_ERROR("tdav_codec_ulpfec_dec_deserialize Failed to create levels");
    	return tsk_null;
    }
    
    pkt->hdr.L  = (pdata[0] >> 6) & 0x01;
    pkt->hdr.SN_base.value =((pdata[2] << 8)&0xff00) + (pdata[3] & 0xff);
    pkt->hdr.TS = ((pdata[4] << 24)  & 0xff000000)+ ((pdata[5] << 16)& 0x00ff0000) + ((pdata[6] << 8)&0xff00) + pdata[7];

    pkt->hdr.length = ((pdata[8] << 8) & 0xff00) + pdata[9];

    // skip header
    pdata+= 10;
    
    if (!(level0 = tsk_object_new(tdav_fec_level_def_t))){
        TSK_DEBUG_ERROR("tdav_codec_ulpfec_dec_deserialize failed to create level");
        return NULL;
    }

    protect_length = ((pdata[0]<< 8)&0xff00) + pdata[1];

    TSK_DEBUG_INFO("fec parse pkt->hdr.TS:%d, pkt->hdr.SN_base.value:%d, hdr.len:%d, protect_length:%d", pkt->hdr.TS, pkt->hdr.SN_base.value, pkt->hdr.length, protect_length);
    
    level0->payload.ptr = tsk_malloc(protect_length);
    if (level0->payload.ptr  == tsk_null){
        TSK_DEBUG_ERROR("tdav_codec_ulpfec_dec_deserialize failed to create payload");
        return NULL;
    }
    level0->hdr.mask_size = 16;//self->encoder.pkt->hdr.L ? 48 : 16;

    //skip ulp header
    pdata += 4;

    // payload
    memcpy(level0->payload.ptr, pdata , protect_length);

    // add level to list
    temp = level0;
    tsk_list_push_back_data(pkt->levels, (void**)&temp);

    return pkt;
}

tsk_size_t tdav_codec_ulpfec_enc_serialize(const tdav_codec_ulpfec_t* self, void** out_data, tsk_size_t* out_max_size)
{
	uint8_t* pdata;
	tsk_size_t xsize;
	int32_t i;
	tsk_list_item_t* item;
	tdav_fec_level_t* level;

	if (!self || !self->encoder.pkt || !out_data){
		TSK_DEBUG_ERROR("Invalid parameter");
		return 0;
	}
	xsize = tdav_codec_ulpfec_guess_serialbuff_size(self);

	if (*out_max_size < xsize){
		if (!(*out_data = tsk_realloc(*out_data, xsize))){
			TSK_DEBUG_ERROR("Failed to reallocate buffer with size =%d", xsize);
			*out_max_size = 0;
			return 0;
		}
		*out_max_size = xsize;
	}
	pdata = (uint8_t*)*out_data;

	// E(1), L(1), P(1), X(1), CC(4)
	/*
	pdata[0] =
		(self->encoder.pkt->hdr.E << 7) |
		(self->encoder.pkt->hdr.L << 6) |
		(self->encoder.pkt->hdr.P << 5) |
		(self->encoder.pkt->hdr.X << 4) |
		(self->encoder.pkt->hdr.CC & 0x0F);*/
	pdata[0] = 0xff;//special flag
	// M(1), PT(7)
	pdata[1] = (self->encoder.pkt->hdr.M << 7) | (self->encoder.pkt->hdr.PT & 0x7F);
	// SN base (16)
	pdata[2] = (self->encoder.pkt->hdr.SN_base.value >> 8);
	pdata[3] = (self->encoder.pkt->hdr.SN_base.value & 0xFF);
	// TS (32)
	pdata[4] = self->encoder.pkt->hdr.TS >> 24;
	pdata[5] = (self->encoder.pkt->hdr.TS >> 16) & 0xFF;
	pdata[6] = (self->encoder.pkt->hdr.TS >> 8) & 0xFF;
	pdata[7] = (self->encoder.pkt->hdr.TS & 0xFF);
	// Length (16)
	pdata[8] = (self->encoder.pkt->hdr.length >> 8);
	pdata[9] = (self->encoder.pkt->hdr.length & 0xFF);

	pdata += 10;

	tsk_list_foreach(item, self->encoder.pkt->levels){
		if (!(level = item->data)){
			continue;
		}
		// Protection length (16)
		pdata[0] = (level->hdr.length >> 8);
		pdata[1] = (level->hdr.length & 0xFF);
		pdata += 2;
		// mask (16 or 48)
		for (i = (int32_t)(level->hdr.mask_size - 8); i >= 0; i -= 8){
			*pdata = ((level->hdr.mask >> i) & 0xFF); ++pdata;
		}
		// payload
		memcpy(pdata, level->payload.ptr, level->hdr.length);
	}

	return xsize;
}



static int tdav_codec_ulpfec_open(tmedia_codec_t* self)
{
	return 0;
}

static int tdav_codec_ulpfec_close(tmedia_codec_t* self)
{
	return 0;
}

static tsk_size_t tdav_codec_ulpfec_encode(tmedia_codec_t* self, const void* in_data, tsk_size_t in_size, void** out_data, tsk_size_t* out_max_size)
{
	TSK_DEBUG_ERROR("Not expected to be called");
	return 0;
}

static tsk_size_t tdav_codec_ulpfec_decode(tmedia_codec_t* self, const void* in_data, tsk_size_t in_size, void** out_data, tsk_size_t* out_max_size, const tsk_object_t* proto_hdr)
{
	TSK_DEBUG_ERROR("Not expected to be called");
	return 0;
}

static tsk_bool_t tdav_codec_ulpfec_sdp_att_match(const tmedia_codec_t* codec, const char* att_name, const char* att_value)
{
	return tsk_true;
}

static char* tdav_codec_ulpfec_sdp_att_get(const tmedia_codec_t* self, const char* att_name)
{
	return tsk_null;
}


/* ============ ULPFEC object definition ================= */

/* constructor */
static tsk_object_t* tdav_codec_ulpfec_ctor(tsk_object_t * self, va_list * app)
{
	tdav_codec_ulpfec_t *ulpfec = self;
	if (ulpfec){
	    TSK_DEBUG_INFO("tdav_codec_ulpfec_ctor create");
		/* init base: called by tmedia_codec_create() */
		/* init self */
		if (!(ulpfec->encoder.pkt = tsk_object_new(tdav_fec_pkt_def_t))){
			TSK_DEBUG_ERROR("Failed to create FEC packet");
			return tsk_null;
		}
	}
	return self;
}
/* destructor */
static tsk_object_t* tdav_codec_ulpfec_dtor(tsk_object_t * self)
{
	TSK_DEBUG_INFO("tdav_codec_ulpfec_dtor destructor");
	tdav_codec_ulpfec_t *ulpfec = self;
	if (ulpfec){
		/* deinit base */
		tmedia_codec_video_deinit(ulpfec);
		/* deinit self */
		TSK_OBJECT_SAFE_FREE(ulpfec->encoder.pkt);
	}

	return self;
}
/* object definition */
static const tsk_object_def_t tdav_codec_ulpfec_def_s =
{
	sizeof(tdav_codec_ulpfec_t),
	tdav_codec_ulpfec_ctor,
	tdav_codec_ulpfec_dtor,
	tmedia_codec_cmp,
};
/* plugin definition*/
static const tmedia_codec_plugin_def_t tdav_codec_ulpfec_plugin_def_s =
{
	&tdav_codec_ulpfec_def_s,

	tmedia_video,
	tmedia_codec_id_video_fec, // fake codec
	"ulpfec",
	"ulpfec codec",
	TMEDIA_CODEC_FORMAT_ULPFEC,
	tsk_true,
	90000, // rate

	/* audio */
	{ 0 },

	/* video (defaul width,height,fps) */
	{ 176, 144, 15 },

	tsk_null, // set()
	tdav_codec_ulpfec_open,
	tdav_codec_ulpfec_close,
	tdav_codec_ulpfec_encode,
	tdav_codec_ulpfec_decode,
	tdav_codec_ulpfec_sdp_att_match,
	tdav_codec_ulpfec_sdp_att_get
};
const tmedia_codec_plugin_def_t *tdav_codec_ulpfec_plugin_def_t = &tdav_codec_ulpfec_plugin_def_s;