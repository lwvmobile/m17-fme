/*-------------------------------------------------------------------------------
 * m17_rf_encoder.c
 * Project M17 - RF Audio Encoder
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
// #include "m17.h"

//convert bit array into symbols and RF/Audio
void encodeM17RF (config_opts * opts, pa_state * pa, uint8_t * input, float * mem, int type)
{

  //dibits-symbols map
  const int8_t symbol_map[4] = {+1, +3, -1, -3};

  float m17_rrc[81] =
  {
    -0.003195702904062073f, -0.002930279157647190f, -0.001940667871554463f,
    -0.000356087678023658f,  0.001547011339077758f,  0.003389554791179751f,
    0.004761898604225673f,  0.005310860846138910f,  0.004824746306020221f,
    0.003297923526848786f,  0.000958710871218619f, -0.001749908029791816f,
    -0.004238694106631223f, -0.005881783042101693f, -0.006150256456781309f,
    -0.004745376707651645f, -0.001704189656473565f,  0.002547854551539951f,
    0.007215575568844704f,  0.011231038205363532f,  0.013421952197060707f,
    0.012730475385624438f,  0.008449554307303753f,  0.000436744366018287f,
    -0.010735380379191660f, -0.023726883538258272f, -0.036498030780605324f,
    -0.046500883189991064f, -0.050979050575999614f, -0.047340680079891187f,
    -0.033554880492651755f, -0.008513823955725943f,  0.027696543159614194f,
    0.073664520037517042f,  0.126689053778116234f,  0.182990955139333916f,
    0.238080025892859704f,  0.287235637987091563f,  0.326040247765297220f,
    0.350895727088112619f,  0.359452932027607974f,  0.350895727088112619f,
    0.326040247765297220f,  0.287235637987091563f,  0.238080025892859704f,
    0.182990955139333916f,  0.126689053778116234f,  0.073664520037517042f,
    0.027696543159614194f, -0.008513823955725943f, -0.033554880492651755f,
    -0.047340680079891187f, -0.050979050575999614f, -0.046500883189991064f,
    -0.036498030780605324f, -0.023726883538258272f, -0.010735380379191660f,
    0.000436744366018287f,  0.008449554307303753f,  0.012730475385624438f,
    0.013421952197060707f,  0.011231038205363532f,  0.007215575568844704f,
    0.002547854551539951f, -0.001704189656473565f, -0.004745376707651645f,
    -0.006150256456781309f, -0.005881783042101693f, -0.004238694106631223f,
    -0.001749908029791816f,  0.000958710871218619f,  0.003297923526848786f,
    0.004824746306020221f,  0.005310860846138910f,  0.004761898604225673f,
    0.003389554791179751f,  0.001547011339077758f, -0.000356087678023658f,
    -0.001940667871554463f, -0.002930279157647190f, -0.003195702904062073f
  };

  //NOTE: Reorganized type numbers as following:
  //Single Digit numbers 1,2,3,4 are LSF, STR, BRT, and PKT
  //Double Digit numbers 11,33,55,99 are preamble, EOT, or Dead Air

  int i, j, k, x;

  //Preamble A - 0x7777 (+3, -3, +3, -3, +3, -3, +3, -3)
  uint8_t m17_preamble_a[16] = {0,1,1,1, 0,1,1,1, 0,1,1,1, 0,1,1,1};

  //Preamble B - 0xEEEE (-3, +3, -3, +3, -3, +3, -3, +3)
  uint8_t m17_preamble_b[16] = {1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0};

  //EOT Marker - 0x555D
  uint8_t m17_eot_marker[16] = {0,1,0,1, 0,1,0,1, 0,1,0,1, 1,1,0,1};

  //LSF frame sync pattern - 0x55F7 +3, +3, +3, +3, -3, -3, +3, -3
  uint8_t m17_lsf_fs[16] = {0,1,0,1, 0,1,0,1, 1,1,1,1, 0,1,1,1};

  //STR frame sync pattern - 0xFF5D (-3, -3, -3, -3, +3, +3, -3, +3)
  uint8_t m17_str_fs[16] = {1,1,1,1, 1,1,1,1 ,0,1,0,1, 1,1,0,1};

  //PKT frame sync pattern - 0x75FF (-3, -3, -3, -3, +3, +3, -3, +3)
  uint8_t m17_pkt_fs[16] = {0,1,1,1, 0,1,0,1,1,1,1,1,1,1,1,1};

  //BRT frame sync pattern - 0xDF55 (-3, +3, -3, -3, +3, +3, +3, +3)
  uint8_t m17_brt_fs[16] = {1,1,0,1, 1,1,1,1, 0,1,0,1, 0,1,0,1};

  //load bits into a dibit array plus the framesync bits
  uint8_t output_dibits[192]; memset (output_dibits, 0, sizeof(output_dibits));

  //Preamble (just repeat the preamble 12 times to make 192 symbols)
  if (type == 11) //A Pattern prepends LSF (last symbol opposite of first symbol to prevent zero-crossing)
  {
    for (i = 0; i < 192; i++)
      output_dibits[i] = (m17_preamble_a[ (i*2+0)%16 ] << 1) + (m17_preamble_a[ (i*2+1)%16 ] << 0);
  }

  //Preamble (just repeat the preamble 12 times to make 192 symbols)
  if (type == 33) //B Pattern prepends BRT (last symbol opposite of first symbol to prevent zero-crossing)
  {
    for (i = 0; i < 192; i++)
      output_dibits[i] = (m17_preamble_b[ (i*2+0)%16 ] << 1) + (m17_preamble_b[ (i*2+1)%16 ] << 0);
  }

  //EOT Marker (just repeat the EOT marker 12 times to make 192 symbols)
  if (type == 55)
  {
    for (i = 0; i < 192; i++)
      output_dibits[i] = (m17_eot_marker[ (i*2+0)%16 ] << 1) + (m17_eot_marker[ (i*2+1)%16 ] << 0);
  }

  //load frame sync pattern
  if (type == 1) //LSF
  {
    for (i = 0; i < 8; i++)
      output_dibits[i] = (m17_lsf_fs[i*2+0] << 1) + (m17_lsf_fs[i*2+1] << 0);
  }

  if (type == 2) //Stream
  {
    for (i = 0; i < 8; i++)
      output_dibits[i] = (m17_str_fs[i*2+0] << 1) + (m17_str_fs[i*2+1] << 0);
  }

  if (type == 3) //BRT
  {
    for (i = 0; i < 8; i++)
      output_dibits[i] = (m17_brt_fs[i*2+0] << 1) + (m17_brt_fs[i*2+1] << 0);
  }

  if (type == 4) //PKT
  {
    for (i = 0; i < 8; i++)
      output_dibits[i] = (m17_pkt_fs[i*2+0] << 1) + (m17_pkt_fs[i*2+1] << 0);
  }

  //load rest of frame (if not preamble, EOT marker, or dead air)
  if (type < 5)
  {
    for (i = 0; i < 184; i++)
      output_dibits[i+8] = (input[i*2+0] << 1) + (input[i*2+1] << 0);
  }

  //convert to symbols
  int output_symbols[192]; memset (output_symbols, 0, 192*sizeof(int));
  for (i = 0; i < 192; i++)
    output_symbols[i] = symbol_map[output_dibits[i]];

  //symbols to audio

  //upsample 10x
  int output_up[192*10]; memset (output_up, 0, 192*10*sizeof(int));
  for (i = 0; i < 192; i++)
  {
    for (j = 0; j < 10; j++)
      output_up[(i*10)+j] = output_symbols[i];
  }

  //craft baseband with deviation + filter
  short baseband[1920]; memset (baseband, 0, 1920*sizeof(short));

  //simple, no filtering
  if (opts->disable_rrc_filter == 1)
  {
    for (i = 0; i < 1920; i++)
      baseband[i] = output_up[i] * 7168.0f;
  }
  
  //version w/ filtering lifted from M17_Implementations / libM17
  else if (opts->disable_rrc_filter == 0)
  {
    
    float mac = 0.0f;
    x = 0;
    for (i = 0; i < 192; i++)
    {
      mem[0] = (float)output_symbols[i] * 7168.0f;

      for (j = 0; j < 10; j++)
      {

        mac = 0.0f;

        //calc the sum of products
        for (k = 0; k < 81; k++)
          mac += mem[k]*m17_rrc[k]*sqrtf(10.0);

        //shift the delay line right by 1
        for (k = 80; k > 0; k--)
          mem[k] = mem[k-1];

        mem[0] = 0.0f;

        baseband[x++] = (short)mac;
      }

    }
  }

  //dead air type, output to all enabled formats zero sample to simulate dead air
  //NOTE: 25 rounds is approximately 1 second even, seems optimal
  if (type == 99)
  {
    memset (output_dibits, 0xFF, sizeof(output_dibits)); //NOTE: 0xFF works better on bin files
    memset (baseband, 0, 1920*sizeof(short));
  }

  //save symbols (dibits, actually) to symbol capture bin file format
  // if (opts->symbol_out_f) //use -c output.bin to use this format (default type for DSD-FME)
  // {
  //   for (i = 0; i < 192; i++)
  //     fputc (output_dibits[i], opts->symbol_out_f);
  // }

  //save symbol stream format (M17_Implementations), output to float values that m17-packet-decode can read
  // if (opts->use_dsp_output) //use -Q output.bin to use this format, will be placed in the DSP folder (reusing DSP)
  // {
  //   FILE * pFile; //file pointer
  //   pFile = fopen (opts->dsp_out_file, "a"); //append, not write
  //   float val = 0;
  //   for (i = 0; i < 192; i++)
  //   {
  //     val = (float)output_symbols[i];
  //     fwrite(&val, 4, 1, pFile);
  //   }
  //   fclose(pFile);
  // }

  //playing back signal audio into device/udp
  //NOTE: Open the analog output device, use -8
  if (1 == 1)
  {
    //Pulse Audio
    if (pa->pa_output_rf_is_open == 1)
      // pa_simple_write(opts->pulse_raw_dev_out, baseband, 1920*2, NULL);
      pulse_audio_output_rf(pa, baseband, 1920);
    
    // // UDP
    // if (opts->audio_out_type == 8)
    //   udp_socket_blasterA (opts, state, 1920*2, baseband);

    // //STDOUT or OSS 48k/1
    // if (opts->audio_out_type == 1 || opts->audio_out_type == 5)
    //   write (opts->audio_out_fd, baseband, 1920*2);

  }
  
  //if we have a raw signal wav file, write to it now
  // if (opts->wav_out_raw != NULL)
  // {
  //   sf_write_short(opts->wav_out_raw, baseband, 1920);
  //   sf_write_sync (opts->wav_out_raw);
  // }

  //NOTE: Internal voice decoding is disabled when tx audio over a hardware device, wav/bin still enabled
  // UNUSED(opts);

}

