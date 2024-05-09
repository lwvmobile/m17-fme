
# M17 - Florida Man Edition

## How to Build

### Dependencies

Install Dependencies. Although M17-FME can be built with only the most minimal dependencies to make it highly modular and also highly portable, it is highly recommended to install all dependencies if possible, or there will be no nicer features like Ncurses Terminal w/ KB shortcuts, pulse audio input and output, and no Codec2 Support (which if you want M17 voice, you need Codec2).

The following assumes a Debian/Ubuntu/Linux Mint Operating System, the following has been tested on Linux Mint 23.1 as of 2024.05.09. Please see your OS repo or package manager for equivalent packages if using RHEL/Arch/Other based environments.

```
recommended:
sudo apt install cmake make build-essential git wget libsndfile1-dev libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol socat

required:
sudo apt install cmake make build-essential git libsndfile1-dev

optional:
sudo apt install libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol wget socat

```

NOTE: Users can see [DSD-FME](https://github.com/lwvmobile/dsd-fme/blob/audio_work/examples/Install_Notes.md#manual-install "DSD-FME") for Fedora and Arch, as the above requirements are also listed there, be sure to skip any undeeded ones such as mbelib, ITPP, rtlsdr, etc, as those are not used here. If you currently are a user of DSD-FME, you can just skip installing any of these (except Codec2) since you will already have the prerequisite dependencies installed for this as well.

### Pull and Install (Manual)

```
git clone https://github.com/lwvmobile/m17-fme.git
cd m17-fme
mkdir build
cd build
cmake ..
make
sudo make install
```

### Automatic Build Script

//TODO: Make a build script for debian/*buntu/mint and arch based on pre-existing ones for DSD-FME.

## License

