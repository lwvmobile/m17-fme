/*-------------------------------------------------------------------------------
 * init_super.c
 * Project M17 - Super (Nested) Structure Element Initialization
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//initialize configuration options
void init_super (Super * super)
{
  //init_config_opts
  super->opts.a = 0;
  sprintf (super->opts.b, "%s", "initialize");
  super->opts.c[0] = 0;
  memset (super->opts.d, 0, sizeof(super->opts.d));

  //Generic Options For Display and Logging
  super->opts.use_ncurses_terminal = 0;
  super->opts.ncurses_is_open = 0;
  super->opts.ncurses_no_banner = 0;
  super->opts.use_ncurses_terminal = 0;
  super->opts.ncurses_no_history = 0;

  //Pulse Audio User Options
  super->opts.use_pa_input = 1;
  super->opts.use_pa_output_rf = 0;
  super->opts.use_pa_output_vx = 0;

  //STDIN
  super->opts.use_stdin_input = 0;

  //M17 Encoder and Decoder Options
  super->opts.use_m17_str_encoder = 0;
  super->opts.use_m17_pkt_encoder = 0;
  super->opts.use_m17_brt_encoder = 0;
  super->opts.use_m17_str_decoder = 0;
  super->opts.use_m17_pkt_decoder = 0;
  super->opts.use_m17_ipf_encoder = 0;
  super->opts.use_m17_ipf_decoder = 0;

  //Misc Options to organize later
  super->opts.m17_str_encoder_dt = 2; //2 is fullrate (3200), 3 is halfrate (1600) w/ arb data
  super->opts.disable_rrc_filter = 1; //Disable this later on
  super->opts.monitor_encode_internally = 0;
  super->opts.allow_crc_failure = 0; //allow decode attempts, even if CRC16 fails checksum
  super->opts.use_hpfilter_dig = 1;
  super->opts.stdout_pipe = 0;
  super->opts.use_float_symbol_output = 0;
  sprintf (super->opts.float_symbol_output_file, "m17_float_symbol_out.sym");
  super->opts.float_symbol_out = NULL;

  //UDP for IP frame input or output
  super->opts.m17_use_ip = 0;
  super->opts.m17_portno = 17000;
  super->opts.m17_udp_sock = 0;
  sprintf (super->opts.m17_hostname, "%s", "127.0.0.1");
  sprintf (super->opts.m17_udp_input, "%s", "");
  //end init_config_opts

  //init_pa_state
  #ifdef USE_PULSEAUDIO
  super->pa.input.format = PA_SAMPLE_S16NE;
  super->pa.input.channels = 1;
  super->pa.input.rate = 48000;

  super->pa.output_rf.format = PA_SAMPLE_S16NE;
  super->pa.output_rf.channels = 1;
  super->pa.output_rf.rate = 48000;

  super->pa.output_vx.format = PA_SAMPLE_S16NE;
  super->pa.output_vx.channels = 1;
  super->pa.output_vx.rate = 48000;

  super->pa.inputlt.fragsize = 960*5;
  super->pa.inputlt.maxlength = -1;
  super->pa.inputlt.prebuf = -1;
  super->pa.inputlt.tlength = -1;

  super->pa.outputlt.fragsize = 960*5;
  super->pa.outputlt.maxlength = -1;
  super->pa.outputlt.prebuf = -1;
  super->pa.outputlt.tlength = -1;

  #endif

  super->pa.pa_input_is_open = 0;
  super->pa.pa_output_rf_is_open = 0;
  super->pa.pa_output_vx_is_open = 0;
  //end init_pa_state

  //init_demod_state -- haven't even started yet, so skipping for now
  memset (super->demod.float_sample_buffer, 0, 65535*sizeof(short));
  memset (super->demod.sample_buffer, 0, 65535*sizeof(short));
  super->demod.sample_buffer_ptr = 0;

  memset (super->demod.symbol_buffer, 0, 65535*sizeof(int16_t));
  super->demod.symbol_buffer_ptr = 0;

  memset (super->demod.dibit_buffer, 0, 65535*sizeof(uint8_t));
  super->demod.dibit_buffer_ptr = 0;

  super->demod.carrier = 0;
  super->demod.in_sync = 0;

  super->demod.input_sql = 100;
  super->demod.input_rms = 0;
  //end init_demod_state

  //init_m17d_state (Decoder)
  super->m17d.src = 0;
  super->m17d.dst = 0;
  super->m17d.can = -1;

  memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  memset(super->m17d.meta, 0, sizeof(super->m17d.meta));
  super->m17d.dt = 0;
  super->m17d.enc_et = 0;
  super->m17d.enc_st = 0;
  sprintf (super->m17d.dst_csd_str, "%s", "");
  sprintf (super->m17d.src_csd_str, "%s", "");
  
  super->m17d.reflector_module = 0;

  #ifdef USE_CODEC2
  super->m17d.codec2_3200 = codec2_create(CODEC2_MODE_3200);
  super->m17d.codec2_1600 = codec2_create(CODEC2_MODE_1600);
  #endif
  //end init_m17d_state (Decoder)

  //init_m17e_state (Encoder)
  super->m17e.src = 0;
  super->m17e.dst = 0;
  super->m17e.can = 7;

  super->m17e.enc_et = 0;
  super->m17e.enc_st = 0;
  //'A', single letter reflector module A-Z, 0x41 is A
  super->m17e.reflector_module = 0x41;

  #ifdef USE_CODEC2
  super->m17e.codec2_3200 = codec2_create(CODEC2_MODE_3200);
  super->m17e.codec2_1600 = codec2_create(CODEC2_MODE_1600);
  #endif

  //User Supplied Input Strings
  sprintf (super->m17e.user, "%s", "");
  sprintf (super->m17e.srcs, "%s", "N0CALL");
  sprintf (super->m17e.dsts, "%s", "ALL");
  sprintf (super->m17e.sms, "%s", "");
  sprintf (super->m17e.dat, "%s", "");
  sprintf (super->m17e.arb, "%s", "1234567 ABCDEFG 7654321 GFEDCBA 0000000 ZZZZZZZ");

  //Stream Voice Mode
  super->m17e.str_encoder_tx = 1;
  super->m17e.str_encoder_eot = 0;
  super->m17e.str_encoder_vox = 0;
  //end init_m17e_state (Encoder)

  //init_wav_state
  sprintf (super->wav.wav_out_file_rf, "%s", "m17_rf_wav.wav");
  sprintf (super->wav.wav_out_file_vx, "%s", "m17_vx_wav.wav");
  //end init_wav_state

  //HPF Initception
  //TODO: Sort this out
  //do I have these backwards in DSD-FME, just going to set them up as it is there for now
  // HPFilter_Init(HPFilter *filter, float cutoffFreqHz, float sampleTimeS)
  // HPFilter_Init(HPFilter *filter, float cutoffFreqHz, float sampleTimeS)
  HPFilter_Init (&super->hpf_d, 960, (float)1/(float)48000);
  HPFilter_Init (&super->hpf_a, 960, (float)1/(float)48000);
  //end HPF Init
  
}