
# M17 - Florida Man Edition

## Usage

This is the current usage (subject to change), verbatim, from m17-fme -h option:

```
Usage: m17-fme [options]    Start the Program
  or:  m17-fme -h           Show Help

Display Options:
  -N            Use NCurses Terminal
                 m17-fme -N 2> log.txt 
  -v <num>      Payload Verbosity Level
  -d <num>      Demodulator Verbosity Level

Input Options:

  -i <device>   Audio input device (default is pulserf)
                pulserf for pulse audio RFA input 
                pulsevx for pulse audio Voice / Mic input
                - for STDIN input (specify encoder or decoder options below)
                (Note: When using STDIN, Ncurses Keyboard Shortcuts Disabled)
                (padsp wrapper required for OSS audio on Linux)
                /dev/dsp for OSS audio
                udp for UDP Frame Input (default localhost:17000)
                udp:192.168.7.8:17001 for M17 UDP/IP bind input (Binding Address and Port)
                m17udp:192.168.7.8:17001 for M17 UDP/IP bind input (Binding Address and Port)
  -w <file>     48k/1 SNDFile Compatible RF Audio .wav or .rrc input file
  -c <file>     DSD-FME Compatible Dibit/Symbol Capture Bin input file (from RF Encoder)
  -f <file>     Float Symbol input file (from RF Encoder and M17_Implementations)

Output Options:

  -o <device>   Audio output device (default is pulsevx)
                pulserf for pulse audio RFA output
                pulsevx for pulse audio Voice / Loopback output
                - for STDOUT output (specify encoder or decoder options below)
                (Note: Don't use Ncurses Terminal w/ STDOUT enabled)
                (padsp wrapper required for OSS audio on Linux)
                /dev/dsp for OSS audio
                (OSS Can only do either RF output, or VX output,
                (not both at the same time, specify encoder and decoder options below)
                udp for UDP Frame Output (default localhost:17000)
                udp:192.168.7.8:17001 for M17 UDP/IP blaster output (Target Address and Port)
                m17udp:192.168.7.8:17001 for M17 UDP/IP blaster output (Target Address and Port)
  -W <file>     48k/1 SNDFile Compatible RF Audio .wav output file
  -C <file>     DSD-FME Compatible Dibit/Symbol Capture Bin output file
  -F <file>     Float Symbol output file (M17_Implementations Compatible)

Encoder Options:

  -V            Enable the Stream Voice Encoder
  -P            Enable the Packet Data  Encoder
  -I            Enable IP Frame Output with defaults (can be combined with Loopback or RFA output)
                (can be combined with Loopback or RFA output)
  -L            Enable Internal Encoder Loopback Decoder (must be used with pulsevx output)
  -X            Enable Voice Activated TX (Vox) on Stream Voice Encoder
Encoder Input Strings:

  -M <str>      Set M17 CAN:SRC:DST 
                (example: -M 1:N0CALL:SP5WWP) 
  -U <str>      Set UDP/IP Frame HOST:PORT:MODULE 
                (example: -U 127.0.0.1:17001:B) 
  -S <str>      Enter SMS Message (up to 772 UTF-8 characters) for Packet Data Encoder
                (example: -S 'Hello World! This is a text message' 
  -A <str>      Enter SMS Message (Up to 48 UTF-8 characters) For Stream Voice Encoder (Arbitrary Data). Enables 1600 mode.
                (example: -A 'Hello World! This is arbitrary data on 1600' 
  -R <str>      Enter RAW Data for Packet Data Encoder (TODO: Not Working Yet).
                (example: -R 'however this ends up getting in here, update this line' 
  -x            Encode Inverted Polarity on RF Output

Decoder Options:
  -r            Enable RFA Demodulator and Decoding of Stream and Packet Data
  -x            Expect Inverted Polarity on RF Input
  -u            Enable UDP IP Frame Decoder and Connect to default localhost:17000 
  -p <file>     Per Call decoded voice wav file saving into current directory ./M17WAV folder

Encryption Options:
                (NOTE: Encoder and Decoder share same values here)
  -e <hex>      Enter Scrambler Key Value (up to 24-bit / 6 Hex Character)
                (example: -e ABCDEF)
  -E <hex str>  Enter AES Key Value (in single quote, space every 16 chars) 
                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC 5C34A197764C147A 15FBA7515ED8BCFC')
                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC')
                (NOTE: Due to bug in m17-tools handling of AES keys, all keys are run as AES-128)
                (Limiting significant key value to first 32 characters to maintain compatibility)

Debug Options:
  -1            Generate Random One Time Use 24-bit Scrambler Key 
  -2            Disable RRC Filter on RF Audio Encoding / Decoding. 
  -3            Generate Random One Time Use 24-bit Scrambler Key. 
  -4            Permit Data Decoding on CRC Failure (not recommended). 
  -6            Open All Pulse Input / Output and IP Frame Defaults and Send Voice Stream. (Fire Everything!). 
  -8            Disable High Pass Filter on CODEC2 Output. 
  -9            Enable RRC Filter on RF Audio Encoding / Decoding. 

Quick Examples:

 Stream Voice Encoder with Mic Input (pulsevx) RF Output (pulserf), float symbol file output (float.sym) 
 m17-fme -i pulsevx -o pulserf -V -F float.sym -N 2> m17encoder.txt 
  (Note: When Using Ncurses Terminal with Encoding and Not Vox, use '\' key to toggle TX)

 RF Demodulator for Stream Voice and Data Packet with Decoded Voice Output (pulsevx) 
 m17-fme -i pulserf -o pulsevx -r -N 2> m17decoder.txt 

 Stream Voice Encoder with Mic Input (pulsevx) IP Frame Output 
 m17-fme -i pulsevx -o udp -V -N 2> m17encoder.txt 

 IP Frame Decoder for Voice Stream and Packet Data Default Host and Port 
 m17-fme -i udp -u -o pulsevx -N 2> m17decoder.txt 

 Packet Data Encoder with SMS Message to IP Frame Output to custom port and RF Audio Output
 m17-fme -o pulserf -P -S 'This is a text message' -M 1:M17-FME:ALL -I -U 127.0.0.1:17001:A 

 IP Frame Decoder for Voice Stream and Packet Data Bound to Custom Host and Port 
 m17-fme -i udp:127.0.0.1:17001 -N 2> m17decoder.txt 

```


## License

