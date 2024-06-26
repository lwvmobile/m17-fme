
# M17 Project - Florida Man Edition

## Information

M17-FME is a stand-alone encoder and decoder of [M17 Project](https://m17project.org/ "M17") protocol.
M17-FME uses the open source [Codec2](https://github.com/drowe67/codec2 "Codec2") vocoder, and built on the specifications openly and freely provided under [M17 Protocol Specifications](https://spec.m17project.org/ "M17 Protocol Specifications"). M17-FME should only be considered an 'educational' tool for radio enthusiasts and people interested in tinkering. It is also in an early beta stage, and as such, is prone to error and may have changing functionality over time as features are added, removed, reworked, etc. This tool should never be used for any commercial or critical needs, and any use for ill or malicious intent (jamming, DDOS, trolling, warfare, etc) is NOT CONDONED or TOLERATED!

![M17-FME](https://github.com/lwvmobile/m17-fme/blob/main/docs/m17-fme1.png)

![M17-FME](https://github.com/lwvmobile/m17-fme/blob/main/docs/m17-fme2.png)

## Functionality

### Voice Stream

Voice Stream Encoding and Decoding (RF Audio). Voice Stream Frames can be decoded from any RF Audio source, and M17-FME supports reading in from a Pulse Audio Input Source, OSS Input Source "/dev/dsp", SB16LE 48k/1 STDIN Input, TCP Linked SB16LE 48k/1 Network Source [SDR++](https://github.com/AlexandreRouma/SDRPlusPlus "SDR++"), sndfile compatible SB16LE 48k/1 .wav and .rrc files generated by any compatible M17 project source or recorded from OTA from an actual radio, float symbol input files generated by [M17_Implementations](https://github.com/M17-Project/M17_Implementations "M17_Implementations"), and dibit capture format (capture.bin) generated by [DSD-FME](https://github.com/lwvmobile/dsd-fme "DSD-FME").

When operating in encoding mode, Voice Stream can be modulated in FSK4 48k/1 SB16LE RF Audio output to Pulse Audio Sink, OSS Audio Sink, STDOUT, .wav file, float symbol output file, or dibit output file. Also, while in Encoding Mode, encoded data can also be sent back into the decoder via a loopback functionality which will, in turn, decode the encoded bitstream, with audio output being either decoded voice output, encoded RF output, or both through seperate output sinks, along with any file formats specified.

When operating in decoding mode, RF audio can be captured and saved with floating symbol output file, and decoded voice can be saved on a 'per call' basis with wav file creation for each new call after a no-sync period.

Voice Encoding and Decoding support both Codec2 3200 bps mode "full rate" and Codec2 1600 bps "half rate" modes, per specification. To encode in 1600 bps mode, the user on needs to specify some Arbitrary Data to the encoder, which is in turn handled as an embedded SMS message of up to 48 UTF-8 Characters to be decoded every superframe.

It should also be noted, that M17-FME does not support simultaneous encoding and decoding, as in, you open one session to encode, and can have a second to decode, but one session cannot receive RF audio and encode (transmit) when not receiving. This functionality may be introduced in future updates. M17-FME does however, support a loopback mode where encoded data can be sent back into the decoder side to be decoded as its encoded. Keep in mind, only pulse audio, or a combination of output to audio sink and file will allow both sending of RF audio AND encoded voice or data playback simultaneously.

### Packet Data

Packet Data Encoding and Decoding (RF Audio). Same input and output methods as listed above are available for Packet Data. Currently, only SMS text message protocol is expressely supported by Packet Data Encoding, while all standardized protocols (GNSS, etc) are decoded via the decoder if from another source. Users can encode an SMS message of up to 772 characters. Users can also enter raw encoded packet data as a string of hex octets to be encoded by the packet encoder (up to 772 octets).

### UDP/IP Frame Format

M17-FME is capable of transmitting and receiving UDP frames based on the specification linked above. This input and output has NOT BEEN TESTED on currently operating reflectors, and its status as working is unknown. M17-FME can however, communicate over UDP/IP Protocol to another M17-FME session. Also, note, that M17-FME can only encode and send IP frames, or receive and decode IP frames. Currently, there is no mode that handles simultaneous operation. IP Frame format can be encoded and sent over UDP either alone, or in conjunction with RF encoded audio output.

NOTE: Encodng and Decoding of Packet Data over IP Frames has not been standardized in the M17 specifications, but operates with the same idea in mind as Voice Stream, where one UDP datagram is crafted with a complete LSF Link Data Setup, and the entire Packet Data contents are delivered all in one go.

### Encryption

M17-FME supports both the encryption and decryption of Voice Stream using AES and Scrambler modes per specification. M17-FME also (experimentally and unofficially) supports encryption and decryption of Packet Data using AES and Scrambler modes. The official stance from M17-FME is that encryption should be used as a tool, and in the context of M17-FME, it is available as a learning tool. When using encryption mode, M17-FME can craft and send your encryption key and encryption type as both a Packet, and embedded into Link Setup Data periodically in the clear. "Over the Air Key Delivery" or "OTAKD" was devised as a method to both allow and learn form the use of encryption, but to also freely and openly provide the encryption key to others to use while decoding. This 'format' is NOT per M17 specification, but is a method devised internally to allow the use of encryption while sharing the key for others. OTAKD can be enabled by using the `-O` command line option, or using the `O` or `o` keyboard shortcut in the Ncurses Terminal.

### secp256r1 Signatures with Private and Public Keys

An emerging standard in M17 is the use of private and public keys to generate secp256r1 signatures and attach them to the tail end of transmissions. M17-FME can both encode and decode secp256r1 signatures using either private keys for encoding, public keys for decoding, or a combination of both. Users need only to generate and specify keys to be imported with the `-k` and `-K` option. See the included key folder for example keys. Keys can be generated in multiple ways, for example, using the command: `openssl ecparam -name prime256v1 -genkey -noout -out keys.pem && openssl pkey -in keys.pem -text > keys.txt && rm keys.pem` and then copy and pasting the resulting public and private key into txt files like in the example, or by using the `-5` command line option. Note: Using the first method mentioned, on the public key, discard the first octet of the public key, or octet 0x04.

Similar to OTAKD for Encryption, OTASK format has been created for delivering signature public keys over the air. OTASK can be enabled at the command line via '-Q' option, or enabled or sent one time via the 'P' and 'p' options. Random Key Pairs can be generated at the command line via the `-3` option as mentioned above and used during session, or generated for future use with the `-5` command line option. Simialrly, Key Pairs can be generated in the Ncurses Terminal with the `-3` keyboard shortcut, and enabled or sent one time with the `P` and `p` keys.

### How to Use

Please see [Example Usage](https://github.com/lwvmobile/m17-fme/blob/main/docs/Example_Usage.md "Example Usage") for a complete set of use case scenarios and configuration.

### How to Build

Please see [Install Notes](https://github.com/lwvmobile/m17-fme/blob/main/docs/Install_Notes.md "Install Notes") for information on cloning and compiling M17-FME.

