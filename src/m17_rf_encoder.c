/*-------------------------------------------------------------------------------
 * m17_rf_encoder.c
 * Project M17 - RF Audio Encoder
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//convert bit array into symbols and RF/Audio
void encode_rfa (Super * super, uint8_t * input, float * mem, int type)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  //NOTE: type numbers as following:
  //Single Digit numbers 1,2,3,4 are LSF, STR, BRT, and PKT
  //Double Digit numbers 11,33,55,99 are preamble, EOT, or Dead Air

  int i, j, k, x; UNUSED(k); UNUSED(x);

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
  if (super->opts.inverted_signal)
  {
    for (i = 0; i < 192; i++)
      output_symbols[i] = inv_symbol_map[output_dibits[i]];
  }
  else
  {
    for (i = 0; i < 192; i++)
      output_symbols[i] = symbol_map[output_dibits[i]];
  } 
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
  if (super->opts.disable_rrc_filter == 1)
  {
    for (i = 0; i < 1920; i++)
      baseband[i] = output_up[i] * 7168.0f;
  }

  //version w/ filtering lifted from M17_Implementations / libM17
  else if (super->opts.disable_rrc_filter == 0)
    upsacale_and_rrc_output_filter (output_symbols, mem, baseband);
  
  //version w/ filtering lifted from M17_Implementations / libM17
  // else if (super->opts.disable_rrc_filter == 0)
  // {
    
  //   float mac = 0.0f;
  //   x = 0;
  //   for (i = 0; i < 192; i++)
  //   {
  //     mem[0] = (float)output_symbols[i] * 7168.0f;

  //     for (j = 0; j < 10; j++)
  //     {

  //       mac = 0.0f;

  //       //calc the sum of products
  //       for (k = 0; k < 81; k++)
  //         mac += mem[k]*m17_rrc[k]*sqrtf(10.0);

  //       //shift the delay line right by 1
  //       for (k = 80; k > 0; k--)
  //         mem[k] = mem[k-1];

  //       mem[0] = 0.0f;

  //       baseband[x++] = (short)mac;
  //     }

  //   }
  // }

  //Apply Gain to Output
  output_gain_rf (super, baseband, 1920);

  //dead air type, output to all enabled formats zero sample to simulate dead air
  //NOTE: 25 rounds is approximately 1 second even, seems optimal
  if (type == 99)
  {
    memset (output_dibits, 0xFF, sizeof(output_dibits)); //NOTE: 0xFF works better on bin files
    memset (baseband, 0, 1920*sizeof(short));
  }

  //save dibits to DSD-FME compatible "symbol" capture bin file format
  if (super->opts.dibit_out) //use -C output.bin to use this format for output
  {
    for (i = 0; i < 192; i++)
      fputc (output_dibits[i], super->opts.dibit_out);
  }

  //save symbol stream format (M17_Implementations), if opened
  if (super->opts.float_symbol_out)
  {
    float val = 0;
    for (i = 0; i < 192; i++)
    {
      val = (float)output_symbols[i];
      fwrite(&val, sizeof(float), 1, super->opts.float_symbol_out); //sizeof(float) is 4 (usually)
    }
  }

  //STDOUT (if not internally decoding or using rf stream decoder)
  if (super->opts.stdout_pipe && super->opts.monitor_encode_internally == 0)
    write_stdout_pipe(super, baseband, 1920);

  //OSS output (if not internally decoding or using rf stream decoder)
  if (super->opts.use_oss_output == 1 && super->opts.monitor_encode_internally == 0)
    oss_output_write(super, baseband, 1920);

  //don't send 'dead air' out over pulse audio devices, or may incur some lag
  if (type != 99)
  {
    //Pulse Audio (if RF Stream is open)
    #ifdef USE_PULSEAUDIO
    if (super->pa.pa_output_rf_is_open == 1)
      pulse_audio_output_rf(super, baseband, 1920);
    #endif

  }

  //write to rf wav file
  if (super->wav.wav_out_rf != NULL)
  {
    write_wav_out_rf(super, baseband, 1920);
    sf_write_sync (super->wav.wav_out_rf);
  }

}

