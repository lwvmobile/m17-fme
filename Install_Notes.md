
# M17 - Florida Man Edition

## Auto Install Scripts

M17-FME has auto install scripts for Debian/Ubuntu/Mint based distros, Red Hat/Fedora/RHEL based distors, and for Arch based distros. Simply downloading and running these scripts will download all dependencies and install for you.

Debian/Ubuntu/Mint/Raspberry Pi OS
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/download-and-install-deb.sh
sh download-and-install-deb.sh
```

Red Hat/Fedora/RHEL
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/download-and-install-rhel.sh
sh download-and-install-rhel.sh
```

Arch
```
wget https://raw.githubusercontent.com/lwvmobile/m17-fme/main/download-and-install-arch.sh
sh download-and-install-arch.sh
```

## How to Build (Maunual Install)

### Dependencies

Install Dependencies. Although M17-FME can be built with only the most minimal dependencies to make it highly modular and also highly portable, it is highly recommended to install all dependencies if possible, or there will be no nicer features like Ncurses Terminal w/ KB shortcuts, pulse audio input and output, and no Codec2 Support (which if you want M17 voice, you need Codec2).

Debian/Ubuntu/Mint/Raspberry Pi OS
```
sudo apt update

recommended:
sudo apt install cmake make build-essential git wget libsndfile1-dev libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol socat

required:
sudo apt install cmake make build-essential git libsndfile1-dev

optional:
sudo apt install libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol wget socat

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

### Pull, Compile, and Install

```
git clone https://github.com/lwvmobile/m17-fme.git
cd m17-fme
mkdir build
cd build
cmake ..
make
sudo make install
```


