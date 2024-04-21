#ifndef MAIN_H
#define MAIN_H

//defined by CMakeLists.txt -- Enable by using cmake -DCOLORS=ON ..
#ifdef PRETTY_COLORS
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#else
#define KNRM  ""
#define KRED  ""
#define KGRN  ""
#define KYEL  ""
#define KBLU  ""
#define KMAG  ""
#define KCYN  ""
#define KWHT  ""
#endif

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

  uint8_t disable_rrc_filter;

  int stdout_pipe;

  //UDP for IP frame output
  int m17_use_ip;     //if enabled, open UDP and broadcast IP frame
  int m17_portno;    //default is 17000
  int m17_udp_sock; //actual UDP socket for M17 to send to
  char m17_hostname[1024];

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

  #ifdef USE_CODEC2
  struct CODEC2 *codec2_3200;
  struct CODEC2 *codec2_1600;
  #endif

} m17_decoder_state;

typedef struct
{
  unsigned long long int src;
  unsigned long long int dst;

  #ifdef USE_CODEC2
  struct CODEC2 *codec2_3200;
  struct CODEC2 *codec2_1600;
  #endif

} m17_encoder_state;

//Pulse Audio Options and States
typedef struct
{
  #ifdef USE_PULSEAUDIO
  pa_sample_spec input;
  pa_buffer_attr inputlt;
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
void upsample_6x(short input, short * output);

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

//M17
void encodeM17RF (config_opts * opts, pa_state * pa, wav_state * wav, uint8_t * input, float * mem, int type);
void encodeM17PKT(config_opts * opts, pa_state * pa, wav_state * wav);

//if using cpp code, then put function prototypes in below
#ifdef __cplusplus
extern "C" {
#endif

void sample_cpp_func(uint8_t * input);

#ifdef __cplusplus
}
#endif

#endif // MAIN_H
