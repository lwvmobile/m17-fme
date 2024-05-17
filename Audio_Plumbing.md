
# M17 Project - Florida Man Edition

## Audio Plumbing

While using M17-FME to encode/modulate and decode/demodulate RF Audio, you will find it necessary to properly plumb (or route) audio from each input source and output sink properly. This mini tutorial aims to help you do such when using Pulse Audio.

You will first want to locate and run the virtualsink.sh script file via `sh virtualsink.sh` command. This will create two null-sink, or 'virtual sinks' for us to plumb audio from point A to point B. 

Here are screenshots of a proper setup using pavucontrol, or "Pulse Audio Volume Control", having voice input from the encoder listening to your microphone, the RF Output going into the "M17_Sink" null-sink, and on the playback side, playing back voice to our speakers, with RF modulated audio being monitored on "M17-Sink" null-sink. You will find that session id values are available for easy matching.

![Audio 1](https://github.com/lwvmobile/m17-fme/blob/main/pavucontrol_plumbing1.png)

![Audio 2](https://github.com/lwvmobile/m17-fme/blob/main/pavucontrol_plumbing2.png)

The general idea is that we want to route RF modulated audio into our M17_Sink and have our decoder listen to the same sink. Voice Input should always be from a microphone source and Voice Output should always be to Audio Hardware (speakers, headphones, etc).

Note, that using the virtualsink.sh will create these sinks for us for the duration of your computers boot. If you reboot, they will not persist. Configuration files can be modified to always have these sinks available, or the `virtualsink.sh` script can be executed on login, but you must do so at your own risk and own research, it is out of the scope of this tutorial to create persistent sinks.

In case of the usage of a headless environment and pavucontrol is not viable, you may use alternatives such as pulsemixer which is similar in functionality, just as a CLI tool and not a GUI tool.