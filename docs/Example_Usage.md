
# M17 Project - Florida Man Edition

## Using M17-FME with M17-FME (Standalone)

### Simple Examples

The simplest method of using M17-FME is to use it as an encoder, and activate the enternal loopback decoder to encode a bitstream, and then feed the encoded bitstram back into the decoder. The loopback method currently only supports Voice Stream encoding. This can be done with the following command.

```
m17-fme -i pulsevx -o pulsevx -V -L -N 2> m17encoder.txt
```

Similarly, A quick simple test to have m17-fme encode and modulate packet data and send it back into itself to be decoded can be seen below. Note: When using the -P option to encode, and not specifying any data, the default SMS text message "Lorem Ipsum..." is encoded and sent.

```
m17-fme -P -o - 2> /dev/null | m17-fme -i - -r
```

### RF Modulated and Demodulated Voice and Data

Using the example below, you can create a voice stream encoding session which will listen to voice input 'pulsevx', output rf modulated audio to output sink 'pulserf', and create a float symbol output file capable of being replayed by multiple M17 related software packages (M17_Implementations, M17-FME, others). Furthermore, Ncurses Terminal is enabled, and the console output is routed to a log file m17encoder.txt. The -V (capitalized V) is used to enable the Voice Stream encoder.

```
m17-fme -i pulsevx -o pulserf -V -F float.sym -N 2> m17encoder.txt
```

The modulated RF audio output can in turn, be demodulated and decoded with the following command. The -r option is used to enable the RF Audio Demodulator.

```
m17-fme -i pulserf -o pulsevx -r -N 2> m17decoder.txt
```

Packet Data can likewise be encoded and modulated for the above decoder to decode by using the following command. Note the use the -P (capitalized P) for Packet Encoder, and the -S followed by the SMS Text Message, which must be in quotations.

```
m17-fme -o pulserf -P -F float.sym -S "this is a simple SMS text message sent over M17 Packet Data."
```

Please see [Audio Plumbing](https://github.com/lwvmobile/m17-fme/blob/main/docs/Audio_Plumbing.md "Audio Plumbing") for general help with audio plumbing (routing from A to B).

### IP Frame Voice and Data

Using setups similar to above, or extra switches in conjunction with above examples, IP Frame output can be added to any RF Audio Encoding Session, or used exlusively. For example, we can quickly add the -I option to the Voice Stream encoder option above to enable IP Frame output to the default host and port (localhost:17000). Users can specify a custom hostname and port for encoding and decoding using the -U option, `-U 192.168.7.5:17001` noting both an encoding and decoding session must both have the same argument to be able to send and receive.

Here is an example of using IP Frame with Voice Stream Encoding, setting a custom port to 17001, setting custom M17 User Can to 6, SRC to M17-FME, and DST to ALL (broadcast 0xFFFFFFFFFFFF) and setting a 24-bit scrambler key of value 0x123456.

Encode:

```
m17-fme -i pulsevx -V -I -U localhost:17001 -M 6:M17-FME:ALL -e 123456 -N 2> m17encoder.txt
```

Decode:

```
m17-fme -o pulsevx -u -U localhost:17001 -M 6:M17-FME:ALL -e 123456 -N 2> m17decoder.txt
```

Other acceptable methods of input and output for udp are using the -i and -o options udp such that `-i upp` or `-i udp:localhost:17001`, see full usage below for more.

### Complex Example

This is a complex setup example such that the encoder will encode Voice Stream and modulate it into RF Audio, send IP Frames to 127.0.0.1:17002, use AES encrytion with key specified, and so on.

Encode:

```
m17-fme -i pulsevx -o pulserf -V -I -U 127.0.0.1:17002 -E '1234567890ABCDEF 1234567890ABCDEF 1234567890ABCDEF 1234567890ABCDEF' -N 2> m17encoder.txt
```

With AES and Scrambler keys, M17-FME utilizes a homebrew method called OTAKD which will deliver the encryption keys and type to listeners (this can be disabled at compile time), the end user can specify keys if they wish, or just allow them to arrive from OTAKD during encoding. Note the omission of the AES key on the IP Frame decoder, the OTAKD packet and Embedded Link Data will alert the decoder to the key used and configure it appropriately.

Decode(IP):

```
m17-fme -i udp:127.0.0.1:17002 -o pulsevx -N 2> m17decoder.txt
```

Decode(RF):

```
m17-fme -i pulserf -o pulsevx -r -E '1234567890ABCDEF 1234567890ABCDEF 1234567890ABCDEF 1234567890ABCDEF' -N 2> m17decoder.txt
```

NOTE: Packet Data can be sent and received over IP Frame just as it is encoded and decoded using RF Audio, but this is not currently an M17 Specification, and is internal to M17-FME. 

## Duplex Mode Operation

M17-FME can now be used in a 'duplex' mode of operation, that is, a single session that can both encode and transmit, AND listen for and decode M17 traffic. This mode is the new preferred method of using M17-FME, if possible, but will absolutely requrie the use of the Ncurses Terminal for Stream Voice TX and Packet Mode Data Entry. Duplex Mode will also require using Pulse Audio for Voice Input, RF Input, or both, due to the need to gracefully open and close audio streams internally on the fly. Duplex Mode can operate in RF input and output mode, or in IP input and output mode, but not both at the same time.

To use M17-FME in RF mode, simply run:

```
m17-fme -D 2> m17duplex.txt
```

To run M17-FME in IP mode, it is recommended to use a target IP address that is the broadcast address on your subnet for LAN opeations. This example readme will not cover the use of IP addressing and subnets, etc, but as a general example for IP 4 addressing, `.255` is the typical broadcast IP address under a subnet of `255.255.255.0`. The example provided below assumes your PC is setup with an address of `192.168.7.X` where X can be any number from 1 to 254. This mode can be used on two or more computers on your LAN to talk back and forth using the example provided below. Note: Input IP is always bound to `localhost:17000` Changing the SRC from `USER1` to different names (or your callsign) is beneficial when using IP frames locally.

To use M17-FME in IP mode, run:

```
m17-fme -D -I -U 192.168.7.255:17000 -M 7:USER1:ALL 2> m17ip.txt
```

## Using M17-FME with M17_Implementations

m17-fme can be used together with m17-decoder-sym such that m17-decoder-sym can read in float symbol input generated by m17-fme as output in real time by tailing its output floatsym.sym file and reading it in and decoding it as m17-fme generates the file.

```
m17-fme -i pulsevx -o pulserf -V -N 2> m17encoder.txt -F ~/floatsym.sym
```

```
tail -f ~/floatsym.sym | ./m17-decoder-sym 
```

Similarly, m17-packet-decode can decode m17-fme generated packet data.

```
m17-fme -P -F ~/floatsym.sym -S 'This is a simple text message sent from m17-fme to m17-packet-decode'
```

```
tail -f ~/floatsym.sym | ./m17-packet-decode
```

m17-fme can take either .rrc, .wav, or float symbol input (preferred) from m17-packet-encode and decode them by doing the following:

```
echo -en "\x05Testing M17 packet mode." | ./m17-packet-encode -S N0CALL -D ALL -C 10 -n 25 -o float.sym -f
```

```
m17-fme -r -f float.sym
```

There are other combinations of usage between M17_Implementations and M17-FME, its up to you to experiment to find them all.

## Using M17-FME with m17-tools

m17-fme can be used together with m17-tools m17-mod, such that m17-mod will modulate audio, and m17-fme will demodulate and decode it. Here is an example of m17-mod modulating voice stream rf audio from a voiced wav input file, and feeding the RF modulated audio directly into m17-fme to be demodulated and decoded.

Please note, the following example uses AES "256" encryption from m17-tools, which is then decrypted by m17-fme using a 128-bit segment of the same key, noting that m17-tools is hard coded to AES 128-bit mode. Recent updates to M17 Protocol also now use the subtype field to signal AES-128, AES-192, or AES-256, and m17-tools is, very conveniently, also signalling AES-128 due to lack of setting the subtype field appropriately.

See the samples folder for the 8k1_voice.wav file.

```
sox 8k1_voice.wav -t raw - |  ./m17-mod -S N0CALL -D N0CALL -r -v -K FFFFFFFFFFFFFFFFAAAAAAAAAAAAAAAACCCCCCCCCCCCCCCCEEEEEEEEEEEEEEEE | m17-fme -i - -r -o pulsevx -E "FFFFFFFFFFFFFFFF AAAAAAAAAAAAAAAA"
```

## Using M17-FME with gr-m17

m17-fme can be used with some of the gr-m17 test tools by modifying or creating a File Sink with input type 'float' in GNU Radio Companion. In this example, a float File Sink node has been created in m17_loopback.grc with the file `float.sym` saved to the home directory, and connected to the output of the M17 Encoder. A terminal has been opened and the output from gr-m17 read in with m17-fme with the command: `m17-fme -r -f ~/float.sym`

![gr-m17](https://github.com/lwvmobile/m17-fme/blob/main/docs/gr-m17_float_symbol_filesink.png)

After this file has been created and populated with float symbols from the example, the example can be stopped, and much like reading float symbol data from other compatible projects, the float data can be read in by using `m17-fme -r -f ~/float.sym`

## Full Usage

This is the current usage (subject to change), verbatim, from m17-fme -h option:

```
Usage: m17-fme [options]    Start the Program
  or:  m17-fme -h           Show Help

Display Options:

  -N            Use NCurses Terminal
                 m17-fme -N 2> log.txt 
  -v <num>      Payload Verbosity Level
  -d <num>      Demodulator Verbosity Level

Device Options:

  -a            List All Pulse Audio Input Sources and Output Sinks (devices).

Input Options:

  -i <device>   Audio input device (default is pulserf)
                pulserf for pulse audio RFA input 
                pulserf:6 or pulserf:m17_sink2.monitor for pulse audio RFA input on m17_sink2 (see -a) 
                pulsevx for pulse audio Voice / Mic input
                pulsevx:2 or pulserf:alsa_input.pci-0000_0d_00.3.analog-stereo for pulse audio Voice / Mic input on device (see -a) 
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
                pulserf:5 or pulserf:m17_sink2 for pulse audio RFA output on m17_sink2 (see -a) 
                pulsevx for pulse audio Voice / Loopback output
                pulsevx:1 or pulserf:alsa_output.pci-0000_0d_00.3.analog-stereo for pulse audio Voice / Loopback output on device (see -a) 
                - for STDOUT output (specify encoder or decoder options below)
                (Note: Don't use Ncurses Terminal w/ STDOUT enabled)
                (padsp wrapper required for OSS audio on Linux)
                /dev/dsp for OSS audio
                (OSS Can only do either RF output, or VX output,
                 not both at the same time, specify encoder and decoder options below)
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
  -L            Enable Internal Encoder Loopback Decoder (must be used with pulsevx output)
  -X            Enable Voice Activated TX (Vox) on Stream Voice Encoder
  -s <dec>      Input Squelch v RMS Level (Vox) on Stream Voice Encoder
  -x            Modulate Inverted Polarity on RF Output
  -K <file>     Load secp256r1 Private Key from file. (see example key: key/sig_pri_key.txt)

Encoder Input Strings:

  -M <str>      Set M17 CAN:SRC:DST 
                (example: -M 1:N0CALL:SP5WWP) 
  -U <str>      Set UDP/IP Frame HOST:PORT:MODULE 
                (example: -U 127.0.0.1:17001:B) 
  -S <str>      Enter SMS Message (up to 821 UTF-8 characters) for Packet Data Encoder
                (example: -S 'Hello World! This is a text message') 
  -A <str>      Enter SMS Message (Up to 48 UTF-8 characters) For Stream Voice Encoder (Arbitrary Data). Enables 1600 mode.
                (example: -A 'Hello World! This is arbitrary data on 1600') 
  -R <hex>      Enter RAW Data for Packet Data Encoder as Hex Octets (up to 823 octets).
                (example: -R 8169001E135152397C0A0000005A45) for Packet GNSS Position @ Wally World) 

                (NOTE: Using Meta Fields is not compatible with Using Encryption!)
  -Y <str>      Enter META Data for Stream Voice Encoder as Text String (Up to 13 UTF-8 characters, single segment only);
                (example: -Y 'Hello World!!') for Meta Text 
  -Z <hex>      Enter META Data for Stream Voice Encoder as Hex Octets (1 Meta Type Octet + 14 Hex Octets Max);
                (example: -Z 0169001E135152397C0A0000005A45) for Meta GNSS Position @ Wally World 

Decoder Options:

  -r            Enable RFA Demodulator and Decoding of Stream and Packet Data
  -x            Demodulate Inverted Polarity on RF Input
  -m            Enable Analog / Raw Input Signal Monitor on RF Input (when no sync)
  -l            Enable Event Log File: date_time_m17fme_eventlog.txt
  -u            Enable UDP IP Frame Decoder and Connect to default localhost:17000 
  -p            Per Call decoded voice wav file saving into current directory ./m17wav folder
  -k <file>     Load secp256r1 Public Key from file. (see example key: key/sig_pub_key.txt)

Duplex Options:

  -D            Enable Duplex Mode (Send and Receive over RF or IP Frame)
                 EXPERIMENTAL! Current Implementation Requires Pulse Audio and Ncurses Availability, Vox Disabled
                 RF Example:
                 m17-fme -D 2> m17e.txt
                 IP Frame Example:
                 LAN Machine 1: m17-fme -D 2> m17e.txt -I -U 192.168.7.255:17000
                 LAN MAchine 2: m17-fme -D 2> m17e.txt -I -U 192.168.7.255:17000

Encryption Options:

                (NOTE: Encoder and Decoder share same values here)
  -e <hex>      Enter Scrambler Key Value (up to 24-bit / 6 Hex Character)
                (example: -e ABCDEF)
  -E <hex str>  Enter AES Key Value (in single quote, space every 16 chars) 
                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC 5C34A197764C147A 15FBA7515ED8BCFC')
                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC')
                (NOTE: Due to bug in m17-tools handling of AES keys, all keys are run as AES-128)
                (Limiting significant key value to first 32 characters to maintain compatibility)
  -J <file>     Load AES Key from file. (see example key: key/aes_key.txt)
  -O            Send OTA Key Delivery Packets and Embedded LSD for AES and Scrambler Keys
  -Q            Send OTA Key Delivery Packets for Signature Public Keys

Debug Options:

  -1            Generate Random One Time Use 24-bit Scrambler Key 
  -2            Generate Random One Time Use 256-bit AES Key. 
  -3            Generate Random Keys For secp256r1 Signatures. Enable Signing and Verification.
  -4            Permit Data Decoding on CRC Failure (not recommended). 
  -5            Generate Random Keys For secp256r1 Signatures, and exit.
  -6            Open All Pulse Input / Output and IP Frame Defaults and Send Voice Stream. (Fire Everything!). 
  -7            Disable Symbol Timing Correction. 
  -8            Disable High Pass Filter on CODEC2 Output. 
  -9            Enable  RRC Filter on RF Audio Encoding / Decoding. 
  -0            Disable RRC Filter on RF Audio Encoding / Decoding. 

Quick Examples:

 Stream Voice Encoder with Mic Input (pulsevx) RF Output (pulserf), float symbol file output (float.sym) 
 m17-fme -i pulsevx -o pulserf -V -F float.sym -N 2> m17encoder.txt 
  (Note: When Using Ncurses Terminal with Encoding and Not Vox, use '\' key to toggle TX)

 RF Demodulator for Stream Voice and Data Packet with Decoded Voice Output (pulsevx) 
 m17-fme -i pulserf -o pulsevx -r -N 2> m17decoder.txt 

 Stream Voice Encoder with Mic Input (pulsevx) IP Frame Output Default Host and Port
 m17-fme -i pulsevx -o udp -V -I -N 2> m17encoder.txt 

 IP Frame Decoder for Voice Stream and Packet Data Default Host and Port 
 m17-fme -i udp -u -o pulsevx -N 2> m17decoder.txt 

 Packet Data Encoder with SMS Message to IP Frame Output to custom port and RF Audio Output
 m17-fme -o pulserf -P -S 'This is a text message' -M 1:M17-FME:ALL -I -U 127.0.0.1:17001:A 

 IP Frame Decoder for Voice Stream and Packet Data Bound to Custom Host and Port 
 m17-fme -i udp:127.0.0.1:17001 -N 2> m17decoder.txt 

```

### Ncurses Keyboard Shortcuts

```

// Voice Stream Functionality
'\' key, Toggle TX Voice Encoder (when not Vox)
'v' key, Toggle Vox Mode Voice Encoder
'h' key, Toggle High Pass Filter on CODEC2 Output
'q' key, Quit

// Packet and Data Functionality
'o' key, send OTAKD Encryption Key Delivery Packet over RF
'p' key, send OTASK Signature Key Delivery Packet over RF


't' key, enter and send SMS Text Message over RF or IP
'u' key, enter and send Raw Data Packet over RF or IP
'w' key, Load Arb Data as Text Message for Stream
'm' key, Load Meta Data as Text Message for Stream

  NOTE: if sending with 't' or 'u' and OTAKD and/or OTASK are enabled, 
  OTAKD and OTASK packets are also delivered via RF and IP

  NOTE: hitting 't', 'u', 'w', or 'm' and entering blank data will
  zero out that field, and will revert 1600+arb to 3200 on 'w'.

// Modulation and Demodulation
'r' key, Toggle RRC Input / Output Filtering
'x' key, Toggle Inversion
'M' key, Toggle Analog / Raw Signal Monitor (when no sync)

// Audio Levels
'/' key, decrement voice input gain by 1%
'*' key, increment voice input gain by 1%

'-' key, decrement voice output gain by 1%
'+' key, increment voice output gain by 1%

'[' key, decrement rf input gain by 1%
']' key, increment rf input gain by 1%

'{' key, decrement rf output gain by 1%
'}' key, increment rf output gain by 1%

'<' key, decrement input squelch by 10
'>' key, increment input squelch by 10

// Encryption and Signatures
'1' key, Generate Random Scrambler Key (24-bit)
'2' key, Generate Random AES Key (256-bit)
'3' key, Generate Random Signature Key Pair and Enable Stream Signing
'5' key, Disable Signature Stream Signing
'E' key, Toggle AES Encryption (only when not TX, and a key is loaded)
'e' key, Toggle Scrambler Encryption (only when not TX, and a key is loaded)
'O' key, Toggle OTA Key Delivery (only when not TX, and a key is loaded)
'P' key, Toggle OTA Signature Key Delivery (only when not TX, and a key is loaded)

// Ncurses Displays
'C' key, Toggle Banner (Capital C)
'I' key, Toggle Input Output Display
'A' key, Toggle Audio Level Display
'S' key, Toggle Symbol Scope Display
'D' key, Toggle Encode / Decode Display
'H' key, Toggle Show Call History
'c' key, Reset Call History (lower c)
'L' key, Print Call History (current) to console / log;

// Terminal/Log Verbosity Levels
'Z' key, Cycle Demodulator Verbosity (1-5)
'z' key, Toggle Payload Verbosity (On/Off)

// Debug Toggles
'4' key, simulate no_carrier_sync (reset states)
'7' key, Toggle Symbol Timing

```
