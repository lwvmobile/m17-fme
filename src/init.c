/*-------------------------------------------------------------------------------
 * init.c
 * Project M17 - Structure Element Initialization
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//initialize configuration options
void init_config_opts (config_opts * opts)
{
  opts->a = 0;
  sprintf (opts->b, "%s", "initialize");
  opts->c[0] = 0;
  memset (opts->d, 0, sizeof(opts->d));

  //Generic Options For Display and Logging
  opts->use_ncurses_terminal = 0;
  opts->payload_verbosity = 0;

  //Pulse Audio User Options
  opts->use_pa_input = 0;
  opts->use_pa_output_rf = 0;
  opts->use_pa_output_vx = 0;

  //M17 Encoder and Decoder Options
  opts->use_m17_str_encoder = 0;
  opts->use_m17_pkt_encoder = 0;
  opts->use_m17_brt_encoder = 0;
  opts->use_m17_str_decoder = 0;
  opts->use_m17_pkt_decoder = 0;
  opts->use_m17_ipf_encoder = 0;
  opts->use_m17_ipf_decoder = 0;

  //Misc Options to organize later
  opts->m17_str_encoder_dt = 2; //2 is fullrate (3200), 3 is halfrate (1600) w/ arb data
  opts->disable_rrc_filter = 1; //Disable this later on
  opts->stdout_pipe = 0;
  opts->use_float_symbol_output = 0;
  sprintf (opts->float_symbol_output_file, "m17_float_symbol_out.sym");
  opts->float_symbol_out = NULL;

  //UDP for IP frame input or output
  opts->m17_use_ip = 0;
  opts->m17_portno = 17000;
  opts->m17_udp_sock = 0;
  sprintf (opts->m17_hostname, "%s", "127.0.0.1");
  sprintf (opts->m17_udp_input, "%s", "");

}

void init_pa_state (pa_state * pa)
{
  #ifdef USE_PULSEAUDIO
  pa->input.format = PA_SAMPLE_S16NE;
  pa->input.channels = 1;
  pa->input.rate = 48000;

  pa->output_rf.format = PA_SAMPLE_S16NE;
  pa->output_rf.channels = 1;
  pa->output_rf.rate = 48000;

  pa->output_vx.format = PA_SAMPLE_S16NE;
  pa->output_vx.channels = 1;
  pa->output_vx.rate = 48000;

  pa->inputlt.fragsize = 960*5;
  pa->inputlt.maxlength = -1;
  pa->inputlt.prebuf = -1;
  pa->inputlt.tlength = -1;

  pa->outputlt.fragsize = 960*5;
  pa->outputlt.maxlength = -1;
  pa->outputlt.prebuf = -1;
  pa->outputlt.tlength = -1;

  #endif

  pa->pa_input_is_open = 0;
  pa->pa_output_rf_is_open = 0;
  pa->pa_output_vx_is_open = 0;
  
}

void init_demod_state (demod_state * demod)
{
  memset (demod->float_sample_buffer, 0, 65535*sizeof(short));
  memset (demod->sample_buffer, 0, 65535*sizeof(short));
  demod->sample_buffer_ptr = 0;

  memset (demod->symbol_buffer, 0, 65535*sizeof(int16_t));
  demod->symbol_buffer_ptr = 0;

  memset (demod->dibit_buffer, 0, 65535*sizeof(uint8_t));
  demod->dibit_buffer_ptr = 0;
}

void init_m17d_state (m17_decoder_state * m17d)
{
  m17d->src = 0;
  m17d->dst = 0;
  m17d->can = -1;

  memset(m17d->lsf, 0, sizeof(m17d->lsf));
  memset(m17d->meta, 0, sizeof(m17d->meta));
  m17d->dt = 0;
  m17d->enc_et = 0;
  m17d->enc_st = 0;
  sprintf (m17d->dst_csd_str, "%s", "");
  sprintf (m17d->src_csd_str, "%s", "");

  #ifdef USE_CODEC2
  m17d->codec2_3200 = codec2_create(CODEC2_MODE_3200);
  m17d->codec2_1600 = codec2_create(CODEC2_MODE_1600);
  #endif
}

void init_m17e_state (m17_encoder_state * m17e)
{
  m17e->src = 0;
  m17e->dst = 0;
  m17e->can = 7;

  #ifdef USE_CODEC2
  m17e->codec2_3200 = codec2_create(CODEC2_MODE_3200);
  m17e->codec2_1600 = codec2_create(CODEC2_MODE_1600);
  #endif

  //User Supplied Input Strings
  sprintf (m17e->user, "%s", "");
  sprintf (m17e->srcs, "%s", "N0CALL");
  sprintf (m17e->dsts, "%s", "ALL");
  sprintf (m17e->sms, "%s", "");
  sprintf (m17e->dat, "%s", "");
  sprintf (m17e->arb, "%s", "1234567 ABCDEFG 7654321 GFEDCBA 0000000 ZZZZZZZ");

  //Stream Voice Mode
  m17e->str_encoder_tx = 1;
  m17e->str_encoder_eot = 0;
}

void init_wav_state (wav_state * wav)
{
  sprintf (wav->wav_out_file_rf, "%s", "m17_rf_wav.wav");
  sprintf (wav->wav_out_file_vx, "%s", "m17_vx_wav.wav");
}