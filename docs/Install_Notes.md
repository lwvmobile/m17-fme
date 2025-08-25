
# M17 Project - Florida Man Edition

## Auto Install Scripts

M17-FME has auto install scripts for Debian/Ubuntu/Mint based distros, Red Hat/Fedora/RHEL based distros, and for Arch based distros. Simply downloading and running these scripts will download all dependencies and install for you.

Debian / Ubuntu (22.04 LTS) / Mint 21 / Raspberry Pi OS
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/scripts/download-and-install-deb.sh
sh download-and-install-deb.sh
```

Debian 12 / Ubuntu 24.04 LTS (and newer)
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/scripts/download-and-install-ubuntu2404lts.sh
sh download-and-install-ubuntu2404lts.sh
```

Red Hat/Fedora/RHEL
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/scripts/download-and-install-rhel.sh
sh download-and-install-rhel.sh
```

Arch
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/scripts/download-and-install-arch.sh
sh download-and-install-arch.sh
```

macOS (Homebrew)
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/scripts/download-and-install-macos.sh
sh download-and-install-macos.sh
```

## How to Build (Manual Install)

### Dependencies

Install Dependencies. Although M17-FME can be built with only the most minimal dependencies to make it highly modular and also highly portable, it is highly recommended to install all dependencies if possible, or there will be no nicer features like Ncurses Terminal w/ KB shortcuts, pulse audio input and output, and no Codec2 Support (which if you want M17 voice, you need Codec2).

Debian / Ubuntu (22.04 LTS and lower) / Mint 21 / Raspberry Pi OS
```
sudo apt update

recommended:
sudo apt install cmake make build-essential git wget libsndfile1-dev libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol socat

required:
sudo apt install cmake make build-essential git libsndfile1-dev

optional:
sudo apt install libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol wget socat

```


Debian 12 / Ubuntu 24.04 LTS (newer libncurses packages)

```
sudo apt update

recommended:
sudo apt install cmake make build-essential git wget libsndfile1-dev libcodec2-dev libncurses-dev libncurses6 libpulse-dev pavucontrol socat

required:
sudo apt install cmake make build-essential git libsndfile1-dev

optional:
sudo apt install libcodec2-dev libncurses-dev libncurses6 libpulse-dev pavucontrol wget socat
```

Red Hat/Fedora/RHEL
```
sudo dnf update

recommended:
sudo dnf install libsndfile-devel pulseaudio-libs-devel cmake git ncurses ncurses-devel gcc wget pavucontrol gcc-c++ codec2-devel

required:
sudo dnf install cmake build-essential git libsndfile-devel gcc-c++ wget

optional:
sudo dnf install codec2-devel ncurses ncurses-devel pulseaudio-libs-devel pavucontrol wget socat

```

Arch (Note, running a full system upgrade is highly advised, or you risk breaking dependency links and borking your system)
```
sudo pacman -Syu

recommended:
sudo pacman -S libpulse cmake ncurses codec2 base-devel libsndfile git wget

required:
sudo pacman install cmake base-devel git libsndfile

optional:
sudo apt install codec2 ncurses libpulse pavucontrol wget socat

```

macOS (Homebrew)
```
# First, install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Update Homebrew
brew update

# Install required dependencies
brew install cmake make git libsndfile

# Install recommended dependencies  
brew install codec2 ncurses pulseaudio pkg-config gcc wget socat

# Note: PulseAudio may need to be started manually:
# brew services start pulseaudio
# or: /opt/homebrew/opt/pulseaudio/bin/pulseaudio --exit-idle-time=-1 --verbose

required:
brew install cmake make git libsndfile

optional:
brew install codec2 ncurses pulseaudio pkg-config gcc wget socat
```

### Pull, Compile, and Install

```
git clone --recursive https://github.com/lwvmobile/m17-fme.git
cd m17-fme
mkdir build
cd build
cmake ..
make
sudo make install
```

### macOS-Specific Notes

When building on macOS, please note the following:

**Audio Support:**
- OSS audio is not available on macOS. Use PulseAudio, file I/O, or network input/output instead.
- PulseAudio may need to be started manually after installation:
  ```
  brew services start pulseaudio
  ```
- For first-time PulseAudio use, you may need to configure audio permissions in System Preferences > Security & Privacy > Microphone.

**Dependencies:**
- All dependencies are available through Homebrew
- The build system automatically detects macOS and adjusts compiler/linker settings
- RPATH is configured to work with Homebrew library locations

**Known Limitations:**
- OSS audio (`/dev/dsp`) is not supported on macOS
- Some warning messages during compilation are normal and don't affect functionality

**Verification:**
After installation, verify M17-FME is working:
```
m17-fme --help
```


