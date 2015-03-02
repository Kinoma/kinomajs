/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef KINOMA_SPEEX_GLUE_H
#define KINOMA_SPEEX_GLUE_H

#define speex_bits_init			kinoma_speex_bits_init
#define speex_bits_reset		kinoma_speex_bits_reset
#define speex_bits_write		kinoma_speex_bits_write
#define speex_bits_nbytes		kinoma_speex_bits_nbytes
#define speex_init_header		kinoma_speex_init_header
#define speex_header_to_packet	kinoma_speex_header_to_packet
#define speex_header_free		kinoma_speex_header_free
#define speex_lib_get_mode		kinoma_speex_lib_get_mode
#define speex_bits_destroy		kinoma_speex_bits_destroy

#define speex_encoder_init		kinoma_speex_encoder_init
#define speex_encode_int		kinoma_speex_encode_int
#define speex_encoder_destroy	kinoma_speex_encoder_destroy
#define speex_encoder_ctl		kinoma_speex_encoder_ctl

#define speex_decoder_init		kinoma_speex_decoder_init
#define speex_decode_int		kinoma_speex_decode_int
#define speex_decoder_destroy	kinoma_speex_decoder_destroy
#define speex_decoder_ctl		kinoma_speex_decoder_ctl
#define speex_bits_read_from	kinoma_speex_bits_read_from

#define speex_nb_mode			kinoma_speex_nb_mode
#define speex_wb_mode			kinoma_speex_wb_mode

#ifdef __cplusplus
extern "C" {
#endif

void kinoma_speex_bits_init(SpeexBits *bits);
int kinoma_speex_encoder_ctl(void *state, int request, void *ptr);
void kinoma_speex_bits_reset(SpeexBits *bits);
int kinoma_speex_bits_write(SpeexBits *bits, char *bytes, int max_len);
int kinoma_speex_bits_nbytes(SpeexBits *bits);
void *kinoma_speex_encoder_init(const SpeexMode *mode);
void kinoma_speex_encoder_destroy(void *state);
int kinoma_speex_encode_int(void *state, spx_int16_t *in, SpeexBits *bits);
void kinoma_speex_init_header(SpeexHeader *header, int rate, int nb_channels, const struct SpeexMode *m);
char *kinoma_speex_header_to_packet(SpeexHeader *header, int *size);
void kinoma_speex_header_free(void *ptr);
void kinoma_speex_bits_destroy(SpeexBits *bits);

void *kinoma_speex_decoder_init(const SpeexMode *mode);
int kinoma_speex_decode_int(void *state, SpeexBits *bits, spx_int16_t *out);
void kinoma_speex_decoder_destroy(void *state);
int kinoma_speex_decoder_ctl(void *state, int request, void *ptr);
void kinoma_speex_bits_read_from(SpeexBits *bits, char *bytes, int len);

void kinoma_speex_init(int mode);

const SpeexMode * kinoma_speex_lib_get_mode (int mode);

extern const SpeexMode kinoma_speex_nb_mode;
extern const SpeexMode kinoma_speex_wb_mode;

#ifdef __cplusplus
}
#endif

#endif
