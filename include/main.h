/*-------------------------------------------------------------------------------
 * main.h
 * Project M17 - Florida Man Edition
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#ifndef MAIN_H
#define MAIN_H

#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h> //OSS?
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

//libsndfile support
#include <sndfile.h>

//OSS support (for Cygwin compatability)
#include <sys/soundcard.h>

//Pulse Audio Support
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/introspect.h>

//Useful Warning Shoosh
#define UNUSED(x)                       ((void)x)

//only include things if found by cmake
#ifdef USE_CODEC2
#include <codec2/codec2.h>
#endif

//RTLSDR support
#ifdef USE_RTLSDR
#include <rtl-sdr.h>
#endif

//Ncurses Support
#ifdef USE_CURSES
#include <locale.h>
#include <ncurses.h>
#endif

//signal handling exitflag
extern volatile uint8_t exitflag;

//General Options
typedef struct
{
  //quick examples
  uint8_t a;
  char b[1024];
  char c[1024];
  uint8_t d[100];

  //Generic Options For Display and Logging
  uint8_t use_ncurses_terminal;
  uint8_t payload_verbosity;

  //Pulse Audio User Options
  uint8_t use_pa_input;
  uint8_t use_pa_output_rf;
  uint8_t use_pa_output_vx;

  //M17 Encoder and Decoder Options
  uint8_t use_m17_str_encoder;
  uint8_t use_m17_pkt_encoder;
  uint8_t use_m17_brt_encoder;
  uint8_t use_m17_str_decoder;
  uint8_t use_m17_pkt_decoder;
  uint8_t use_m17_ipf_encoder;
  uint8_t use_m17_ipf_decoder;

  //Misc Options to organize later
  uint8_t m17_str_encoder_dt;
  uint8_t disable_rrc_filter;
  int stdout_pipe;
  int use_float_symbol_output;
  char float_symbol_output_file[1024];
  FILE * float_symbol_out;

  //UDP for IP frame output
  int m17_use_ip;   //if enabled, open UDP and broadcast IP frame
  int m17_portno;   //default is 17000
  int m17_udp_sock; //actual UDP socket for M17 to send to
  char m17_hostname[1024]; //hostname as a string value
  char m17_udp_input[1024]; //string value of combined input field for udp i.e., localhost:17000

} config_opts;

//Demodulation States
typedef struct
{
  float   float_sample_buffer[65535];
  short   sample_buffer[65535];
  int32_t sample_buffer_ptr;

  int16_t symbol_buffer[65535];
  int32_t symbol_buffer_ptr;

  uint8_t dibit_buffer[65535];
  int32_t dibit_buffer_ptr;

} demod_state;

//M17 Encoder and Decoder States
typedef struct
{
  unsigned long long int src;
  unsigned long long int dst;
  int16_t can;

  uint8_t lsf[240]; //bitwise lsf
  uint8_t meta[16]; //packed meta
  uint8_t dt;
  uint8_t enc_et; //encryption type
  uint8_t enc_st; //encryption sub-type
  char dst_csd_str[50];
  char src_csd_str[50];

  #ifdef USE_CODEC2
  struct CODEC2 *codec2_3200;
  struct CODEC2 *codec2_1600;
  #endif

} m17_decoder_state;

typedef struct
{
  unsigned long long int src;
  unsigned long long int dst;
  int16_t can;

  #ifdef USE_CODEC2
  struct CODEC2 *codec2_3200;
  struct CODEC2 *codec2_1600;
  #endif

  //User Supplied Input Strings
  char user[50]; //user supplied m17 src and dst call sign data
  char srcs[50]; //user supplied m17 src string
  char dsts[50]; //user supplied m17 dst string
  char sms[800]; //user supplied sms text string for pkt encoder
  char dat[800]; //user supplied other data type for pkt encoder
  char arb[800]; //user supplied arbitrary data on 1600

  //Stream Voice Mode
  uint8_t str_encoder_tx; //flag if transmit on or off
  uint8_t str_encoder_eot; //flag if transmit off and send EOT


} m17_encoder_state;

//Pulse Audio Options and States
typedef struct
{
  #ifdef USE_PULSEAUDIO
  pa_sample_spec input;
  pa_buffer_attr inputlt;
  pa_buffer_attr outputlt;
  pa_sample_spec output_rf;
  pa_sample_spec output_vx;

  pa_simple * pa_input_device;
  pa_simple * pa_output_device_rf;
  pa_simple * pa_output_device_vx;
  #endif

  uint8_t pa_input_is_open;
  uint8_t pa_output_rf_is_open;
  uint8_t pa_output_vx_is_open;

} pa_state;

//WAV files with sndfile
typedef struct
{

  SNDFILE *audio_in_file;
  SF_INFO *audio_in_file_info;

  SNDFILE *wav_out_vx;
  SNDFILE *wav_out_rf;

  char wav_out_file_rf[1024];
  char wav_out_file_vx[1024];

} wav_state;

//High Pass Filter
typedef struct
{
    float coef;
    float v_out[2];
    float v_in[2];

} HPFilter;

//c function prototypes

//structure element initialization
void init_config_opts (config_opts * opts);
void init_pa_state (pa_state * pa);
void init_demod_state (demod_state * demod);
void init_m17d_state (m17_decoder_state * m17d);
void init_m17e_state (m17_encoder_state * m17e);
void init_wav_state (wav_state * wav);

//Pulse Audio Handling
#ifdef USE_PULSEAUDIO
void open_pulse_audio_input (pa_state * pa);
void open_pulse_audio_output_rf (pa_state * pa);
void open_pulse_audio_output_vx (pa_state * pa);
void close_pulse_audio_input (pa_state * pa);
void close_pulse_audio_output_rf (pa_state * pa);
void close_pulse_audio_output_vx (pa_state * pa);
short pa_input_read (pa_state * pa);
void pulse_audio_output_rf(pa_state * pa, short * out, size_t nsam);
void pulse_audio_output_vx(pa_state * pa, short * out, size_t nsam);
#endif

//libsndfile Wav File Handling
void open_wav_out_rf (wav_state * wav);
void open_wav_out_vx (wav_state * wav);
void close_wav_out_rf (wav_state * wav);
void close_wav_out_vx (wav_state * wav);
void write_wav_out_rf (wav_state * wav, short * out, size_t nsam);
void write_wav_out_vx (wav_state * wav, short * out, size_t nsam);

//UDP IP Related Functions
int UDPBind (char *hostname, int portno);
int m17_socket_blaster(config_opts * opts, size_t nsam, void * data);
int udp_socket_connectM17(config_opts * opts);
int m17_socket_receiver(config_opts * opts, void * data);

//Audio Manipulation and Filters
long int raw_rms(int16_t *samples, int len, int step);
void upsample_6x(short input, short * output);
void HPFilter_Init(HPFilter *filter, float cutoffFreqHz, float sampleTimeS);
float HPFilter_Update(HPFilter *filter, float v_in);
void hpf(HPFilter * hpf, short * input, int len);

//convolutional encoder and viterbi decoder(s)
void simple_conv_encoder (uint8_t * input, uint8_t * output, int len);
void convolution_decode(uint8_t s0, uint8_t s1);
void convolution_chainback(unsigned char* out, unsigned int nBits);
void convolution_start();
void convolution_init();

//Golay 24_12 encoder and decoder
void Golay_24_12_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Golay_24_12_decode(unsigned char *rxBits);
void Golay_24_12_init();

//CRC16
uint16_t crc16(const uint8_t *in, const uint16_t len);

//demodulation and sync functions
void framesync (config_opts * opts, pa_state * pa, m17_decoder_state * m17d, demod_state * demod);

//misc utility functions
void open_stdout_pipe(config_opts * opts);
void write_stdout_pipe(config_opts * opts, short * out, size_t nsam);
void open_stdin_pipe(wav_state * wav);
short read_stdin_pipe(wav_state * wav);
uint64_t ConvertBitIntoBytes(uint8_t * BufferIn, uint32_t BitLength);

//M17 Frame Encoders
void encodeM17RF (config_opts * opts, pa_state * pa, wav_state * wav, uint8_t * input, float * mem, int type);
void encodeM17PKT(config_opts * opts, pa_state * pa, wav_state * wav, m17_encoder_state * m17e, m17_decoder_state * m17d);
void encodeM17STR(config_opts * opts, pa_state * pa, wav_state * wav, m17_encoder_state * m17e, m17_decoder_state * m17d);

//M17 Content Element Decoders
int  decode_lich_contents(m17_decoder_state * m17d, uint8_t * lich_bits);
void decode_lsf_contents(m17_decoder_state * m17d);
void decode_pkt_contents(uint8_t * input, int len);
void decode_callsign_data(m17_decoder_state * m17d, unsigned long long int dst, unsigned long long int src);

//M17 Frame Demodulators
void demod_lsf(m17_decoder_state * m17d, uint8_t * input, int debug);
void demod_str(m17_decoder_state * m17d, uint8_t * input, int debug);
void prepare_str(m17_decoder_state * m17d, uint8_t * input);

//if using cpp code, then put function prototypes in below
#ifdef __cplusplus
extern "C" {
#endif

//this function has the sqrt function in it, and CMAKE will compile 
//with math library linked if called this way
int sample_cpp_func(int input);

#ifdef __cplusplus
}
#endif

#endif // MAIN_H
