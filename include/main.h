/*-------------------------------------------------------------------------------
 * main.h
 * M17 Project - Florida Man Edition
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#ifndef MAIN_H
#define MAIN_H

#define PI 3.141592653

#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

//libsndfile support
#include <sndfile.h>

//OSS support (for Cygwin compatability)
#include <sys/soundcard.h>

//Pulse Audio Support
#ifdef USE_PULSEAUDIO
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/introspect.h>
#endif

//Useful Warning Shoosh
#define UNUSED(x) ((void)x)

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

#ifdef USE_UECC
#include "../src/micro-ecc/uECC.h"
#endif

//signal handling exitflag
extern volatile uint8_t exitflag;

//General Options
typedef struct
{

  //Ncurses Options
  uint8_t use_ncurses_terminal;
  uint8_t ncurses_is_open;

  uint8_t ncurses_show_banner;
  uint8_t ncurses_show_io;
  uint8_t ncurses_show_audio;
  uint8_t ncurses_show_decode;
  uint8_t ncurses_show_scope;
  uint8_t ncurses_show_history;

  //input and output strings for parsing
  char  input_handler_string[2048];
  char output_handler_string[2048];

  //output verbosity levels
  int payload_verbosity;
  int demod_verbosity;

  //Pulse Audio User Options
  uint8_t use_pa_input;
  uint8_t use_pa_input_vx;
  uint8_t use_pa_output_rf;
  uint8_t use_pa_output_vx;

  //STDIN and STDOUT
  uint8_t use_stdin_input;
  uint8_t use_stdout_output;

  //OSS Input and Output
  uint8_t use_oss_input;
  uint8_t use_oss_output;
  char oss_input_dev_str[1024];
  char oss_output_dev_str[1024];
  int oss_input_device;
  int oss_output_device;

  //SND Input
  uint8_t use_snd_input;
  uint8_t snd_input_is_a_file; //if we are specifying a .wav file, etc, and not stdin or tcp

  //USE WAV OUTPUT
  uint8_t use_wav_out_rf;
  uint8_t use_wav_out_vx;
  uint8_t use_wav_out_pc;

  //M17 Encoder and Decoder Options
  uint8_t use_m17_str_encoder;
  uint8_t use_m17_pkt_encoder;
  uint8_t use_m17_brt_encoder;
  uint8_t use_m17_tst_encoder;
  uint8_t use_m17_rfa_decoder;
  uint8_t use_m17_ipf_encoder;
  uint8_t use_m17_ipf_decoder;
  uint8_t use_m17_duplex_mode;
  uint8_t use_m17_textgame_mode;
  uint8_t use_m17_packet_burst;

  //Misc Options to organize later
  uint8_t m17_str_encoder_dt;
  uint8_t disable_rrc_filter;
  uint8_t internal_loopback_decoder;
  uint8_t allow_crc_failure;
  uint8_t disable_symbol_timing;
  uint8_t use_raw_audio_monitor;
  uint8_t use_hpfilter_dig;
  uint8_t inverted_signal;
  uint16_t random_number;
  int input_sample_rate;
  int output_sample_rate;
  int stdout_pipe;

  //key delivery
  uint8_t use_otakd;
  uint8_t use_otask;

  //Gain
  float input_gain_rf;
  float input_gain_vx;
  float output_gain_rf;
  float output_gain_vx;

  //Input and Output Files
  uint8_t use_float_symbol_output;
  uint8_t use_float_symbol_input;
  uint8_t use_dibit_input;
  uint8_t use_dibit_output;
  char float_symbol_output_file[1024];
  char float_symbol_input_file[1024];
  char dibit_input_file[1024];
  char dibit_output_file[1024];
  FILE * float_symbol_out;
  FILE * float_symbol_in;
  FILE * dibit_in;
  FILE * dibit_out;

  //Key Input Files
  char aes_key_file[1024];
  char pri_key_file[1024];
  char pub_key_file[1024];
  FILE * aes_key;
  FILE * pri_key;
  FILE * pub_key;

  //Event Log
  uint8_t use_event_log;
  char event_log_file[1024];
  FILE * event_log;

  //UDP for IP frame output
  uint8_t m17_use_ip;   //if enabled, open UDP and broadcast IP frame
  int m17_portno;   //default is 17000
  int m17_udp_sock; //UDP socket for M17 to send to
  char m17_hostname[1024]; //hostname as a string value
  char m17_udp_input[1024]; //string value of combined input field for udp i.e., localhost:17000

  //TCP Audio Source Options
  uint8_t use_tcp_input; //if enabled, use
  uint8_t tcp_input_open; //if successfully opened
  int tcp_input_portno;   //default is 7355
  int tcp_input_sock; //TCP socket for TCP Audio Source
  char tcp_input_hostname[1024]; //hostname as a string value
  char tcp_user_input_str[1024]; //string value of combined input field for udp i.e., localhost:7355

  //RIGCTL Options
  uint8_t use_rig_remote; //if enabled, use
  uint8_t rig_remote_open; //if successfully opened
  int rig_remote_portno;   //default is 4532
  int rig_remote_sock; //TCP socket for RIGCTL Remote Control
  char rig_remote_hostname[1024]; //hostname as a string value
  char rig_remote_input_str[1024]; //string value of combined input field for udp i.e., localhost:4532

  //Other

} config_opts;

//Demodulation States
typedef struct
{
  //NOTE: Recast ptrs as uint8_t which will reach 0xFF or 255 and roll over back to 0,
  //and expanded the buffer to just a bit larger than that, so we don't need the modulos
  short   sample_buffer[256];
  uint8_t sample_buffer_ptr;

  uint8_t dibit_buffer[256];
  uint8_t dibit_buffer_ptr;

  float   float_symbol_buffer[256];
  uint8_t float_symbol_buffer_ptr;

  //this buffer is different from the sample buffer, as opposed to being a ring buffer,
  //this will be filled and discharged every 960 samples for raw audio signal monitor
  short    raw_audio_buffer[960];
  uint16_t raw_audio_buffer_ptr;

  //frame sync
  float sync_symbols[8];
  float sync_distance;
  int fsk4_samples_per_symbol;
  int fsk4_sample_center;
  int fsk4_timing_correction;
  char fsk4_timing_string[50];

  //fsk4 symbol levels
  float fsk4_center;
  float fsk4_min;
  float fsk4_max;
  float fsk4_lmid;
  float fsk4_umid;

  //RRC Input Filter Memory
  float rrc_input_mem[81];

  //sync and time
  uint8_t in_sync;
  time_t sync_time;
  time_t current_time;

  long int input_sql;
  long int input_rms;
  float input_level;

} demod_state;

//ECDSA
typedef struct
{
  uint8_t public_key[64];
  uint8_t private_key[32];
  uint8_t curr_stream_pyl[16];
  uint8_t last_stream_pyl[16];
  uint8_t signature[64];
  uint8_t keys_loaded;
} ECDSA;

//M17 Encoder and Decoder Struct
typedef struct
{
  //NOTE: Not all values will be utilized on encoder or decoder
  unsigned long long int dst; //dst as a 48 bit value
  unsigned long long int src; //src as a 48 bit value
  char dst_csd_str[50]; //dst call sign data as a string
  char src_csd_str[50]; //src call sign data as a string
  int16_t can;          //channel access number

  uint8_t lsf[240]; //bitwise lsf
  uint8_t meta[16]; //packed meta
  uint8_t dt;       //stream or packet data type
  uint8_t enc_et;   //encryption type
  uint8_t enc_st;   //encryption sub-type
  uint8_t met_st;   //meta 'sub-type' //TODO: Switch lsf_st values in str encoder using this, so we don't overwrite enc_st when user enables and disables enc and has meta data loaded as well
  uint8_t enc_mute; //enc, muted audio out

  uint8_t reflector_module; //IP Frame reflector module

  //User Supplied Input Strings
  char user[50]; //user supplied m17 src and dst call sign data
  char srcs[50]; //user supplied m17 src string
  char dsts[50]; //user supplied m17 dst string
  char sms[825]; //user supplied sms text string for pkt encoder
  char dat[825]; //user supplied met data as text string for str encoder
  char arb[825]; //user supplied arbitrary data on 1600

  //PKT specific storage and counters
  uint8_t pkt[850]; //bytewise packet
  uint8_t pbc_ptr;  //internal packet block counter
  uint8_t raw[850]; //raw data from PDU that isn't SMS or UTF-8
  uint8_t meta_data[16]; //encoder meta data (that isn't an IV) as uint8_t array
  uint16_t raw_len; //legnth of raw hex user data for PKT encoder
  uint8_t packet_protocol;

  //Stream Voice Mode
  uint8_t str_encoder_tx;  //flag if transmit on or off
  uint8_t str_encoder_eot; //flag if transmit off and send EOT
  uint8_t str_encoder_vox; //flag if use vox mode

  //Call History
  char callhistory[100][500];

  //LSF Backup Copy
  uint8_t lsf_bkp[240];

  #ifdef USE_CODEC2
  struct CODEC2 *codec2_3200;
  struct CODEC2 *codec2_1600;
  #endif

  //ECDSA
  ECDSA ecdsa;

  //Text Game Progression
  uint32_t game_progress;

} M17;

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
  pa_simple * pa_input_device_vx;
  pa_simple * pa_output_device_rf;
  pa_simple * pa_output_device_vx;
  #endif

  uint8_t pa_input_is_open;
  uint8_t pa_input_vx_is_open;
  uint8_t pa_output_rf_is_open;
  uint8_t pa_output_vx_is_open;

  char pa_input_idx[100];
  char pa_invx_idx[100];
  char pa_outrf_idx[100];
  char pa_outvx_idx[100];

} pa_state;

#ifdef USE_PULSEAUDIO
// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_devicelist
{
  uint8_t initialized;
  char name[512];
  uint32_t index;
  char description[256];
} pa_devicelist_t;
#endif

//WAV output files with sndfile
typedef struct
{
  SNDFILE *wav_out_rf;
  SNDFILE *wav_out_vx;
  SNDFILE *wav_out_pc;
  char wav_out_file_rf[1024];
  char wav_out_file_vx[1024];
  char wav_out_file_pc[1024];
  char wav_file_direct[9];
} wav_state;

//Universal sndfile input (TCP, STDIN, WAV, named PIPE, headerless wav files)
typedef struct
{
  char snd_in_filename[1024];
  SNDFILE *audio_in_file;
  SF_INFO *audio_in_file_info;
} snd_src_input;

//High Pass Filter
typedef struct
{
  float coef;
  float v_out[2];
  float v_in[2];

} hpfilter;

//encryption struct
typedef struct
{

  //type and subtype as specified
  uint8_t enc_type;
  uint8_t enc_subtype;

  //Scrambler 8/16/24 Bit LFSR pN Sequence
  uint32_t scrambler_key;     //8, 16, or 24-bit Scrambler Key as a Hexidecimal Number
  uint8_t  scrambler_pn[128]; //bit-wise pN sequence (keystream) of a Scrambler LFSR (6 * 128 bit payloads)

  uint16_t scrambler_fn_d;    //frame number internal to scrambler to check for mismatch on local fn
  uint16_t scrambler_fn_e;    //frame number internal to scrambler to check for mismatch on local fn

  int8_t scrambler_subtype_d; //subtye set to -1 to check if this has already been set to hard value
  int8_t scrambler_subtype_e; //subtye set to -1 to check if this has already been set to hard value

  uint32_t scrambler_seed_d;  //current value of perpetual scrambler seed
  uint32_t scrambler_seed_e;  //current value of perpetual scrambler seed

  //AES 128/192/256
  uint8_t aes_key_is_loaded; //flag whether or not an aes key has been loaded
  unsigned long long int A1; //64-bit Hexidecimal Representation of Chunk 1 of AES Key (128)
  unsigned long long int A2; //64-bit Hexidecimal Representation of Chunk 2 of AES Key (128)
  unsigned long long int A3; //64-bit Hexidecimal Representation of Chunk 3 of AES Key (192)
  unsigned long long int A4; //64-bit Hexidecimal Representation of Chunk 4 of AES Key (256)
  uint8_t aes_key[32]; //bytewise array of a full length AES Key (256-bit max, or 32 Bytes)

} Encryption;

//Error Tracking
typedef struct
{
  uint16_t lsf_hdr_crc_err;
  uint16_t lsf_emb_crc_err;
  uint16_t pkt_crc_err;
  uint16_t ipf_crc_err;
  uint16_t golay_err;
  uint16_t viterbi_err;
  uint16_t bert_err;

} Error;

//Nested Super Struct comprised of all the other ones so I don't 
//have to pass upteen structs around to everywhere
typedef struct
{
  config_opts opts;
  pa_state pa;
  M17 m17d;
  M17 m17e;
  demod_state demod;
  wav_state wav;
  snd_src_input snd_src_in;
  hpfilter hpf_d;
  Encryption enc;
  Error error;
} Super;

//c function prototypes

//structure element initialization
void init_super (Super * super);
void set_default_state(Super * super);

//io string parsing
void parse_input_option_string (Super * super, char * input);
void parse_output_option_string (Super * super, char * output);
void parse_m17_user_string (Super * super, char * input);
void parse_udp_user_string (Super * super, char * input);
void parse_meta_raw_string (Super * super, char * input);
void parse_meta_txt_string (Super * super, char * input);
void parse_pulse_input_string (Super * super, char * input);
void parse_pulse_input_string_dxv (Super * super, char * input);
void parse_pulse_outrf_string (Super * super, char * input);
void parse_pulse_outvx_string (Super * super, char * input);
uint16_t parse_raw_user_string (Super * super, char * input);

//NCurses Terminal
#ifdef USE_CURSES
void open_ncurses_terminal (Super * super);
void close_ncurses_terminal ();
void print_ncurses_terminal (Super * super);
void print_ncurses_banner (Super * super);
void print_ncurses_config (Super * super);
void print_ncurses_call_info (Super * super);
void print_ncurses_levels (Super * super);
void print_ncurses_scope (Super * super);
void print_ncurses_call_history (Super * super);
void input_ncurses_terminal (Super * super, int c);
#endif

//Pulse Audio Handling
#ifdef USE_PULSEAUDIO
void  open_pulse_audio_input (Super * super);
void  open_pulse_audio_input_rf (Super * super);
void  open_pulse_audio_input_vx (Super * super);
void  open_pulse_audio_output_rf (Super * super);
void  open_pulse_audio_output_vx (Super * super);
void  close_pulse_audio_input (Super * super);
void close_pulse_audio_input_vx (Super * super);
void  close_pulse_audio_output_rf (Super * super);
void  close_pulse_audio_output_vx (Super * super);
short pa_input_read (Super * super);
short pa_input_read_vx (Super * super);
void  pulse_audio_output_rf (Super * super, short * out, size_t nsam);
void  pulse_audio_output_vx (Super * super, short * out, size_t nsam);

//pulse sources and sinks
void pa_state_cb(pa_context *c, void *userdata);
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output);
int pulse_list();
#endif

//OSS Garbage Handling
void  open_oss_output (Super * super);
void  open_oss_input (Super * super);
short oss_input_read (Super * super);
void  oss_output_write (Super * super, short * out, size_t nsam);

//sndfile Wav Output File Handling
void open_wav_out_rf (Super * super);
void open_wav_out_vx (Super * super);
void open_wav_out_pc (Super * super);
void close_wav_out_rf (Super * super);
void close_wav_out_vx (Super * super);
void close_wav_out_pc (Super * super);
void write_wav_out_rf (Super * super, short * out, size_t nsam);
void write_wav_out_vx (Super * super, short * out, size_t nsam);
void write_wav_out_pc (Super * super, short * out, size_t nsam);
void setup_percall_filename (Super * super);

//sndfile Input Open and Reading
bool file_snd_audio_source_open (Super * super);
short snd_input_read (Super * super);

//Input and Output Gain
void  input_gain_rf (Super * super, short * input, int len);
void  input_gain_vx (Super * super, short * input, int len);
void output_gain_rf (Super * super, short * input, int len);
void output_gain_vx (Super * super, short * input, int len);

//UDP IP Related Functions
int  udp_socket_bind (char *hostname, int portno);
int  m17_socket_blaster (Super * super, size_t nsam, void * data);
int  udp_socket_connectM17 (Super * super);
int  m17_socket_receiver (Super * super, void * data);
int  m17_socket_receiver_duplex(int m17_udp_socket_duplex, void * data);
void error(char *msg);

//TCP IP Related Functions
#define BUFSIZE 1024
int  tcp_socket_connect (char *hostname, int portno);
bool tcp_socket_send (int sockfd, char *buf);
bool tcp_socket_receive (int sockfd, char *buf);
bool tcp_snd_audio_source_open (Super * super);

//RIGCTL (Remote Control)
bool rigctl_set_modulation_nfm (int sockfd, int bandwidth);
bool rigctl_set_modulation_wfm (int sockfd, int bandwidth);
bool rigctl_set_frequency (int sockfd, long int freq);

//Frequency Tuning Convenience Function
bool tune_to_frequency (Super * super, long int frequency);

//Audio Input Sample Convenience Functuion
short get_short_audio_input_sample (Super * super);

//Input and Output Open and Close Convenience Functions
void open_audio_input (Super * super);
void open_audio_output (Super * super);
void open_file_output (Super * super);
void cleanup_and_exit (Super * super);

//Audio Manipulation and Filters
long int raw_rms (int16_t *samples, int len, int step);
void upsample_6x (short input, short * output);
void raw_audio_monitor (Super * super, short sample);
void hpfilter_init (hpfilter *filter, float cutoffFreqHz, float sampleTimeS);
float hpfilter_update (hpfilter *filter, float v_in);
void hpfilter_d (Super * super, short * input, int len);

//RRC Input and Output Filtering
short rrc_input_filter(float * mem, short sample);
void upscale_and_rrc_output_filter (int * output_symbols, float * mem, short * baseband);

//simple convolutional encoder
void simple_conv_encoder (uint8_t * input, uint8_t * output, int len);

//libm17 magic soft decision based viterbi
#define SYM_PER_PLD 184
void slice_symbols(uint16_t out[2*SYM_PER_PLD], const float inp[SYM_PER_PLD]);
void randomize_soft_bits(uint16_t inp[SYM_PER_PLD*2]);
void reorder_soft_bits(uint16_t outp[SYM_PER_PLD*2], const uint16_t inp[SYM_PER_PLD*2]);
uint32_t viterbi_decode_punctured(uint8_t* out, const uint16_t* in, const uint8_t* punct, const uint16_t in_len, const uint16_t p_len);

//Golay 24_12 encoder and decoder
void golay_24_12_encode (unsigned char *origBits, unsigned char *encodedBits);
bool golay_24_12_decode (unsigned char *rxBits);
void golay_24_12_init ();

//CRC16
uint16_t crc16 (const uint8_t *in, const uint16_t len);

//demodulation and sync functions
void  framesync (Super * super);
float demodulate_and_return_float_symbol (Super * super);
void  buffer_refresh_min_max_center (Super * super);
short basic_sample_selector (Super * super, short * samples);
void  no_carrier_sync (Super * super);

//fsk4 symbol timing
int   symbol_timing (Super * super, short * samples);
short average_sample_calc(short * samples);

//slice and dice symbols and dibits
uint8_t convert_float_symbol_to_dibit_and_store (Super * super, float float_symbol);
float   float_symbol_slicer(Super * super, short sample);
uint8_t digitize_symbol_to_dibit (float symbol);

//dibit gathering convenience wrapper
uint8_t get_dibit (Super * super);

//based off of lib17 math https://github.com/M17-Project/libm17
float eucl_norm (float* in1, int8_t* in2, uint8_t n);
void  push_float_buffer (float * last, float symbol);
int   dist_and_sync (Super * super, float * last);

//frame sync information and debug prints
void print_frame_sync_pattern (Super * super, int type);
void print_debug_information (Super * super);

//stdin and stdout
void open_stdout_pipe (Super * super);
void write_stdout_pipe (Super * super, short * out, size_t nsam);
bool stdin_snd_audio_source_open (Super * super);

//Time and Date Functions
char * get_time();
char * get_time_n(time_t t);
char * get_date();
char * get_date_n(time_t t);

//misc utility functions
uint64_t convert_bits_into_output (uint8_t * input, int len);
void pack_bit_array_into_byte_array (uint8_t * input, uint8_t * output, int len);
void pack_bit_array_into_byte_array_asym (uint8_t * input, uint8_t * output, int len);
void unpack_byte_array_into_bit_array (uint8_t * input, uint8_t * output, int len);
void convert_dibit_array_into_binary_array (uint8_t * input, uint8_t * output, int len);
void left_shift_byte_array (uint8_t * input, uint8_t * output, int len);
void right_shift_byte_array (uint8_t * input, uint8_t * output, int len);
uint16_t convert_string_into_array (char * input, uint8_t * output);

//M17 Frame Encoders
void encode_rfa (Super * super, uint8_t * input, float * mem, int type);
void encode_pkt(Super * super, int mode);
void encode_str (Super * super);
void encode_str_ecdsa(Super * super, uint8_t lich_cnt, uint8_t * m17_lsf, float * mem, int use_ip, int udpport, uint8_t * sid);
//Special Frame / Packet Encoders
void encode_ota_key_delivery_pkt (Super * super, int use_ip, uint8_t * sid, uint8_t enc_type, uint8_t enc_stype);
void encode_ota_key_delivery_emb(Super * super, uint8_t * m17_lsf, uint8_t * lsf_count);

//test pattern generator
void test_pattern_generator (Super * super);

//M17 Content Element Decoders
int  decode_lich_contents (Super * super, uint8_t * lich_bits);
void decode_lsf_contents (Super * super);
void decode_pkt_contents (Super * super, uint8_t * input, int len);
void decode_callsign_data (Super * super, unsigned long long int dst, unsigned long long int src);
void decode_callsign_src(Super * super, unsigned long long int src);
void decode_str_payload(Super * super, uint8_t * payload, uint8_t type, uint8_t lich_cnt);

//M17 Callsign Data Encoder
void encode_callsign_data(Super * super, char * d40, char * s40, unsigned long long int * dst, unsigned long long int * src);

//M17 Frame Demodulators
void demod_lsf (Super * super, uint8_t * input, int debug);
void demod_pkt (Super * super, uint8_t * input, int debug);
void demod_brt (Super * super, uint8_t * input, int debug);
void demod_str (Super * super, uint8_t * input, int debug);
void prepare_str (Super * super, float * sbuf);
void decode_ipf (Super * super);

//M17 Duplex Mode(s)
void m17_duplex_mode (Super * super);

//M17 Text Based Games (WIP)
void m17_text_games (Super * super);
void load_game_advertisement(Super * super, uint32_t input);
void decode_game_sms_gate(Super * super, uint8_t * input, int len);
void generate_game_sms_reply(Super * super, char * input);
void game_text(Super * super);

//Call History and Event Log
void push_call_history (Super * super);
void print_call_history (Super * super);
void event_log_writer  (Super * super, char * event_string, uint8_t protocol);

//Encryption and Decryption
void scrambler_key_init (Super * super, int de);
uint32_t scrambler_seed_calculation(int8_t subtype, uint32_t key, int fn);
uint32_t scrambler_sequence_generator (Super * super, int de);
void aes_ctr_str_payload_crypt (uint8_t * iv, uint8_t * key, uint8_t * payload, int type);
void aes_ctr_pkt_payload_crypt (uint8_t * iv, uint8_t * key, uint8_t * payload, int type);
void aes_key_loader (Super * super);

//ECDSA
void ecdsa_signature_verification (Super * super);
void ecdsa_signature_creation (Super * super);
void ecdsa_signature_debug_keys (Super * super);
void ecdsa_signature_debug_test();
void ecdsa_generate_random_keys(Super * super);

//if using cpp code, then put function prototypes in below
#ifdef __cplusplus
extern "C" {
#endif

//this function has the sqrt function in it, and CMAKE will compile 
//with math library linked if called this way
int sample_cpp_func (int input);

#ifdef __cplusplus
}
#endif

#endif // MAIN_H
