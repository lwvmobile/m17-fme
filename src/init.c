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
}

void init_pa_state (pa_state * pa)
{
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

  pa->pa_input_is_open = 0;
  pa->pa_output_rf_is_open = 0;
  pa->pa_output_vx_is_open = 0;
  
}