#! /bin/bash
#
cdir=$(pwd)
clear
printf "M17 Project: Florida Man Edition - Auto Installer For Debian / Ubuntu / Mint based Distros.
This will install the required and recommended packages, clone, build, and install M17-FME.
This has been tested on Linux Mint 21.3 Veronica and Ubuntu 22.04 LTS.
A full system upgrade is recommended if before installing new dependencies
Please confirm that you wish to preceed by entering y below.\n\n"
read -p "Do you wish to proceed? y/N " ANSWER
ANSWER=$(printf "$ANSWER"|tr '[:upper:]' '[:lower:]')
if [ "$ANSWER" = "y" ]; then

  sudo apt update
  sudo apt install cmake make build-essential git wget libsndfile1-dev libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev socat

  git clone --recursive https://github.com/lwvmobile/m17-fme.git
  cd m17-fme
  mkdir build
  cd build
  cmake ..
  make -j $(nproc)
  sudo make install
  sudo ldconfig

  printf "Any issues, Please report to:\nhttps://github.com/lwvmobile/m17-fme/issues \n\n"

else
  printf "Thank you, have a nice day!\n\n"
fi