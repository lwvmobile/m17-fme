#! /bin/bash
#
cdir=$(pwd)
clear
printf "Project M17: Florida Man Edition - Auto Installer For Debian / Ubuntu / Mint based Distros\n
This will install the required and recommended packages, clone, build, and install M17-FME\n.
This has been tested on Linux Mint 21.3 Veronica and Ubuntu (insert ver here).\n
A full system upgrade is recommended if before installing new dependencies\n
Please confirm that you wish to preceed by entering y below.\n\n"
read -p "Do you wish to proceed? y/N " ANSWER
ANSWER=$(printf "$ANSWER"|tr '[:upper:]' '[:lower:]')
if [ "$ANSWER" = "y" ]; then

  sudo apt update
  sudo apt install cmake make build-essential git wget libsndfile1-dev libcodec2-dev libncurses5 libncurses5-dev libncursesw5-dev libpulse-dev pavucontrol socat

  git clone https://github.com/lwvmobile/m17-fme.git
  cd m17-fme
  mkdir build
  cd build
  cmake ..
  make -j $(nproc)
  sudo make install
  sudo ldconfig

  printf "Any issues, Please report to:\nhttps://github.com/lwvmobile/m17-fme/issues"

else
  printf "Thank you, have a nice day!\n\n"
fi