/*-------------------------------------------------------------------------------
 * init_super.c
 * M17 Project - Super (Nested) Structure Element Initialization
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
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

  //Ncurses Options
  super->opts.use_ncurses_terminal = 0;
  super->opts.ncurses_is_open = 0;

  super->opts.ncurses_show_banner = 1;
  super->opts.ncurses_show_io = 1;
  super->opts.ncurses_show_audio = 0;
  super->opts.ncurses_show_decode = 1;
  super->opts.ncurses_show_scope = 0;
  super->opts.ncurses_show_history = 1;

  //input and output strings for parsing
  memset (super->opts.input_handler_string,  0, 2048*sizeof(char));
  memset (super->opts.output_handler_string, 0, 2048*sizeof(char));

  //output verbosity levels
  super->opts.payload_verbosity = 0;
  super->opts.demod_verbosity = 0;

  //Pulse Audio User Options
  super->opts.use_pa_input = 0;
  super->opts.use_pa_output_rf = 0;
  super->opts.use_pa_output_vx = 0;

  //STDIN and STDOUT
  super->opts.use_stdin_input = 0;
  super->opts.use_stdout_output = 0;

  //OSS Input and Output
  super->opts.use_oss_input = 0;
  super->opts.use_oss_output = 0;
  sprintf (super->opts.oss_input_dev_str, "%s", "/dev/dsp");
  sprintf (super->opts.oss_output_dev_str, "%s", "/dev/dsp");
  super->opts.oss_input_device = 0;
  super->opts.oss_output_device = 0;

  //SND Input
  super->opts.use_snd_input = 0;
  super->opts.snd_input_is_a_file = 0;

  //USE WAV OUTPUT
  super->opts.use_wav_out_rf = 0;
  super->opts.use_wav_out_vx = 0;
  super->opts.use_wav_out_pc = 0;

  //M17 Encoder and Decoder Options
  super->opts.use_m17_str_encoder = 0;
  super->opts.use_m17_pkt_encoder = 0;
  super->opts.use_m17_brt_encoder = 0;
  super->opts.use_m17_tst_encoder = 0;
  super->opts.use_m17_rfa_decoder = 0;
  super->opts.use_m17_ipf_encoder = 0; //this option isn't used, should use it probably instead of use_ip
  super->opts.use_m17_ipf_decoder = 0;

  //Misc Options to organize later
  super->opts.m17_str_encoder_dt = 2; //2 is fullrate (3200), 3 is halfrate (1600) w/ arb data
  super->opts.disable_rrc_filter = 1; //Disabled until working properly with clock recovery
  super->opts.internal_loopback_decoder = 0;
  super->opts.allow_crc_failure = 0; //allow decode attempts, even if CRC16 fails checksum
  super->opts.disable_symbol_timing = 0; //disable corrective symbol timing (debug)
  super->opts.use_raw_audio_monitor = 0; //monitor raw audio if no sync
  super->opts.use_hpfilter_dig = 1;
  super->opts.inverted_signal = 0;
  srand(time(NULL)); //seed a random number for below
  super->opts.random_number = rand() & 0xFFFF; //random 16-bit number for session ID each start up
  super->opts.input_sample_rate = 48000;   //TODO: Make a function that reconfigures this and anything that is set from this
  super->opts.output_sample_rate = 48000;  //TODO: Make a function that reconfigures this and anything that is set from this
  super->opts.stdout_pipe = 0;

  //Gain
  super->opts.input_gain_rf  = 1.0f;
  super->opts.input_gain_vx  = 1.0f;
  super->opts.output_gain_rf = 1.0f;
  super->opts.output_gain_vx = 1.0f;

  //Input and Output Files
  super->opts.use_float_symbol_output = 0;
  super->opts.use_float_symbol_input = 0;
  super->opts.use_dibit_input = 0;
  super->opts.use_dibit_output = 0;
  sprintf (super->opts.float_symbol_output_file, "m17_float_symbol_out.sym");
  sprintf (super->opts.float_symbol_input_file, "m17_float_symbol_out.sym");
  sprintf (super->opts.dibit_input_file, "capture.bin");
  sprintf (super->opts.dibit_output_file, "capture.bin");
  super->opts.float_symbol_out = NULL;
  super->opts.float_symbol_in = NULL;
  super->opts.dibit_in = NULL;
  super->opts.dibit_out = NULL;

  //Event Log
  super->opts.use_event_log = 0;
  sprintf (super->opts.event_log_file, "eventlog.txt");
  super->opts.event_log = NULL;

  //UDP for IP frame input or output
  super->opts.m17_use_ip = 0;
  super->opts.m17_portno = 17000;
  super->opts.m17_udp_sock = 0;
  sprintf (super->opts.m17_hostname, "%s", "127.0.0.1");
  sprintf (super->opts.m17_udp_input, "%s", "");

  //TCP Audio Source Options
  super->opts.use_tcp_input = 0;
  super->opts.tcp_input_open = 0;
  super->opts.tcp_input_portno = 7355;
  super->opts.tcp_input_sock = 0;
  sprintf (super->opts.tcp_input_hostname, "%s", "127.0.0.1");
  sprintf (super->opts.tcp_user_input_str, "%s", "");

  //RIGCTL Options
  super->opts.use_rig_remote = 0;
  super->opts.rig_remote_open = 0;
  super->opts.rig_remote_portno = 4532;
  super->opts.rig_remote_sock = 0;
  sprintf (super->opts.rig_remote_hostname, "%s", "127.0.0.1");
  sprintf (super->opts.rig_remote_input_str, "%s", "");
  //end init_config_opts

  //init_pa_state
  #ifdef USE_PULSEAUDIO
  super->pa.input.format = PA_SAMPLE_S16NE;
  super->pa.input.channels = 1;
  super->pa.input.rate = super->opts.input_sample_rate;

  super->pa.output_rf.format = PA_SAMPLE_S16NE;
  super->pa.output_rf.channels = 1;
  super->pa.output_rf.rate = super->opts.output_sample_rate;

  super->pa.output_vx.format = PA_SAMPLE_S16NE;
  super->pa.output_vx.channels = 1;
  super->pa.output_vx.rate = super->opts.output_sample_rate;

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

  //init_demod_state
  memset (super->demod.sample_buffer, 0, 256*sizeof(short));
  super->demod.sample_buffer_ptr = 0;

  memset (super->demod.dibit_buffer, 0, 256*sizeof(uint8_t));
  super->demod.dibit_buffer_ptr = 0;

  memset (super->demod.float_symbol_buffer, 0.0f, 256*sizeof(float));
  super->demod.float_symbol_buffer_ptr = 0;

  memset (super->demod.raw_audio_buffer, 0, 960*sizeof(short));
  super->demod.raw_audio_buffer_ptr = 0;

  //frame sync and timing recovery
  memset (super->demod.sync_symbols, 0, 8*sizeof(float));
  super->demod.sync_distance = 8.0f;
  super->demod.fsk4_samples_per_symbol = 10;
  super->demod.fsk4_sample_center = 4;
  super->demod.fsk4_timing_correction = 0;
  sprintf (super->demod.fsk4_timing_string, "Symbol Edge Timing: |            |");

  //fsk4 symbol levels
  super->demod.fsk4_center = 0.0f;
  super->demod.fsk4_min = 0.0f;
  super->demod.fsk4_max = 0.0f;
  super->demod.fsk4_lmid = 0.0f;
  super->demod.fsk4_umid = 0.0f;

  //RRC Input Filter Memory
  memset (super->demod.rrc_input_mem, 0, 81*sizeof(float));

  super->demod.in_sync = 0;
  super->demod.sync_time    = time(NULL);
  super->demod.current_time = time(NULL);

  super->demod.input_sql = 600;
  super->demod.input_rms = 0;
  super->demod.input_level = 0.0f;
  //end init_demod_state

  //init m17d (Decoder)
  super->m17d.dst = 0;
  super->m17d.src = 0;
  sprintf (super->m17d.dst_csd_str, "%s", "         ");
  sprintf (super->m17d.src_csd_str, "%s", "         ");
  super->m17d.can = -1;

  memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  memset(super->m17d.meta, 0, sizeof(super->m17d.meta));
  super->m17d.dt = 15;
  super->m17d.enc_et = 0;
  super->m17d.enc_st = 0;
  super->m17d.enc_mute = 0;
  
  //'A', single letter reflector module A-Z, 0x41 is A
  super->m17d.reflector_module = 0;

  //User Supplied Input Strings / Decoded Strings
  sprintf (super->m17d.user, "%s", "");
  sprintf (super->m17d.srcs, "%s", "N0CALL");
  sprintf (super->m17d.dsts, "%s", "ALL");
  sprintf (super->m17d.sms, "%s", "Any Encoded or Decoded SMS Text Messages Appear Here.");
  sprintf (super->m17d.dat, "%s", "Any Encoded or Decoded GNSS POS Messages Appear Here.");
  sprintf (super->m17d.arb, "%s", "Any Encoded or Decoded 1600 Arb Messages Appear Here.");

  //PKT specific storage and counters
  memset (super->m17d.pkt, 0, sizeof(super->m17d.pkt));
  super->m17d.pbc_ptr = 0;
  memset  (super->m17d.raw, 0, sizeof(super->m17d.raw));
  super->m17d.raw_len = 0;

  //Stream Voice Mode (UNUSED on decoder)
  super->m17d.str_encoder_tx  = 0;
  super->m17d.str_encoder_eot = 0;
  super->m17d.str_encoder_vox = 0;

  //Call History
  for (int i = 0; i < 10; i++)
    sprintf (super->m17d.callhistory[i], "%s", "");

  //LSF Backup Copy
  memset (super->m17d.lsf_bkp, 0, 240*sizeof(uint8_t));

  #ifdef USE_CODEC2
  super->m17d.codec2_3200 = codec2_create(CODEC2_MODE_3200);
  super->m17d.codec2_1600 = codec2_create(CODEC2_MODE_1600);
  #endif
  //end init m17d (Decoder)

  //init m17e (Encoder)
  super->m17e.dst = 0;
  super->m17e.src = 0;
  sprintf (super->m17e.dst_csd_str, "%s", "         ");
  sprintf (super->m17e.src_csd_str, "%s", "         ");
  super->m17e.can = 7;

  memset(super->m17e.lsf, 0, sizeof(super->m17d.lsf));
  memset(super->m17e.meta, 0, sizeof(super->m17d.meta));
  super->m17e.dt = 15;
  super->m17e.enc_et = 0;
  super->m17e.enc_st = 0;
  super->m17e.enc_mute = 0;

  //'A', single letter reflector module A-Z, 0x41 is A
  super->m17e.reflector_module = 0x41;

  //User Supplied Input Strings
  sprintf (super->m17e.user, "%s", "");
  sprintf (super->m17e.srcs, "%s", "N0CALL");
  sprintf (super->m17e.dsts, "%s", "ALL");
  sprintf (super->m17e.sms, "%s", "");
  sprintf (super->m17e.dat, "%s", "");
  sprintf (super->m17e.arb, "%s", ""); //Up To 48 UTF-8 Characters of Text Can Appear Here.

  //PKT specific storage and counters (UNUSED on Encoder)
  memset (super->m17e.pkt, 0, sizeof(super->m17e.pkt));
  super->m17e.pbc_ptr = 0;
  memset  (super->m17e.raw, 0, sizeof(super->m17e.raw));
  super->m17e.raw_len = 0;

  //Stream Voice Mode
  super->m17e.str_encoder_tx  = 1;
  super->m17e.str_encoder_eot = 0;
  super->m17e.str_encoder_vox = 0;

  //Call History (UNUSED on Encoder)
  for (int i = 0; i < 10; i++)
    sprintf (super->m17e.callhistory[i], "%s", "");

  //LSF Backup Copy
  memset (super->m17e.lsf_bkp, 0, 240*sizeof(uint8_t));

  #ifdef USE_CODEC2
  super->m17e.codec2_3200 = codec2_create(CODEC2_MODE_3200);
  super->m17e.codec2_1600 = codec2_create(CODEC2_MODE_1600);
  #endif
  //end init m17e (Encoder)

  //init_wav_state
  super->wav.wav_out_rf = NULL;
  super->wav.wav_out_vx = NULL;
  super->wav.wav_out_pc = NULL;
  sprintf (super->wav.wav_out_file_rf, "%s", "m17_rf_wav.wav");
  sprintf (super->wav.wav_out_file_vx, "%s", "m17_vx_wav.wav");
  sprintf (super->wav.wav_out_file_pc, "%s", "");           //no initial filename
  sprintf (super->wav.wav_file_direct, "%s", "./M17WAV");  //default wav file directory
  struct stat st = {0};
  if (stat(super->wav.wav_file_direct, &st) == -1)
  {
    fprintf (stderr, "Creating directory %s to save M17 per call wav files\n", super->wav.wav_file_direct);
    mkdir (super->wav.wav_file_direct, 0700);
  }
  //end init_wav_state

  //init snd_src_input snd_src_in
  sprintf (super->snd_src_in.snd_in_filename, "snd_file.wav");
  super->snd_src_in.audio_in_file = NULL;
  super->snd_src_in.audio_in_file_info = calloc(1, sizeof(SF_INFO));
  super->snd_src_in.audio_in_file_info->samplerate = super->opts.input_sample_rate;
  super->snd_src_in.audio_in_file_info->channels = 1;
  super->snd_src_in.audio_in_file_info->seekable = 0;
  super->snd_src_in.audio_in_file_info->format = SF_FORMAT_RAW|SF_FORMAT_PCM_16|SF_ENDIAN_LITTLE;
  //end snd_src_input snd_src_in

  //init HPF
  HPFilter_Init (&super->hpf_d, 960, (float)1/(float)super->opts.input_sample_rate);
  //end HPF Init

  //init encryption items
  super->enc.enc_type    = 0;
  super->enc.enc_subtype = 0;

  super->enc.scrambler_key = 0;
  memset (super->enc.scrambler_pn, 0, 768*sizeof(uint8_t));
  super->enc.bit_counter_d = 0;
  super->enc.bit_counter_e = 0;

  super->enc.aes_key_is_loaded = 0;
  super->enc.A1 = 0;
  super->enc.A2 = 0;
  super->enc.A3 = 0;
  super->enc.A4 = 0;
  memset (super->enc.aes_key, 0, 32*sizeof(uint8_t));
  //end init enc

  //init error
  super->error.lsf_hdr_crc_err = 0;
  super->error.lsf_emb_crc_err = 0;
  super->error.pkt_crc_err = 0;
  super->error.ipf_crc_err = 0;
  super->error.golay_err = 0;
  super->error.viterbi_err = 0;
  super->error.bert_err = 0;
  //end init error
}

//this function will enable the default starting state (RF decoder w/ pulse audio input and output)
void set_default_state(Super * super)
{
  super->opts.use_m17_rfa_decoder = 1;

  //NOTE: Run echo | gcc -dM -E -
  //to find #defines for an environment

  /* //arch linux
  #define __linux 1
  #define __gnu_linux__ 1
  #define linux 1
  #define __linux__ 1
  */

  /*
  $ echo | g++ -dM -E - | grep CYGWIN
  #define __CYGWIN__ 1
  #define __CYGWIN32__ 1

  */

  #ifdef __CYGWIN__

  super->opts.use_oss_input = 1;
  super->opts.use_oss_output = 1;

  #else //__gnu_linux__ (or any above listen #define)

  super->opts.use_pa_input = 1;
  super->opts.use_pa_output_vx = 1;

  #endif
}