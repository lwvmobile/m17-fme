
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

### Windows Cygwin Builds

Cygwin builds now have an experimental semi-automatic installer, to run, follow steps below:

Note: If you already have DSD-FME compiled with Cygwin64 using this method, you can skip to the second step, or if you prefer to handpick packages to install, use the itemized packages below to guide you.

Open Windows PowerShell (not Command Prompt) and copy and paste all of this in all at once.

```
Invoke-WebRequest https://cygwin.com/setup-x86_64.exe -OutFile setup-x86_64.exe
.\setup-x86_64.exe --packages nano,libpulse-devel,libpulse-mainloop-glib0,libpulse-simple0,libpulse0,pulseaudio,pulseaudio-debuginfo,pulseaudio-equalizer,pulseaudio-module-x11,pulseaudio-module-zeroconf,pulseaudio-utils,sox-fmt-pulseaudio,libusb0,libusb1.0,libusb1.0-debuginfo,libusb1.0-devel,libncurses++w10,libncurses-devel,libncursesw10,ncurses,cmake,gcc-core,gcc-debuginfo,gcc-objc,git,make,socat,sox,sox-fmt-ao,zip,unzip,wget,gcc-g++,libsndfile-devel

```

Pick a Mirror. http://www.gtlib.gatech.edu mirror seems relatively fast. Nurse the Cygwin installer by clicking next and waiting for it to finish. Ignore the warning popup telling you to install libusb from sourceforge. Install the [Zadig](https://zadig.akeo.ie/ "Zadig") driver instead, if you haven't already and have an RTL Dongle. After Cygwin finishes installing, the installer sh script will download and run, be patient, it may also take a little while.

Then:

```
C:\cygwin64\bin\mintty.exe /bin/bash -l -c "wget https://raw.githubusercontent.com/lwvmobile/m17-fme/refs/heads/main/scripts/download-and-install-cygwin.sh; sh download-and-install-cygwin.sh; m17-fme;"

```

After the sh script finishes, m17-fme should open. If not, then double click on the Cygwin Terminal desktop shortcut, and try running `m17-fme`. If you chose to create a portable version, you will find the folder and zip file in the `C:\cygwin64\home\username` directory if using the default cygwin64 folder location.

You can also update your versions by using the cyg_rebuild.sh script `sh cyg_rebuild.sh`.

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


