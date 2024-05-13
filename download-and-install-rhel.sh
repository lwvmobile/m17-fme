#! /bin/bash
#
cdir=$(pwd)
clear
printf "M17 Project: Florida Man Edition - Auto Installer For Red Hat, Fedora, RHEL based Distros.\n
This will install the required and recommended packages, clone, build, and install M17-FME\n.
This has been tested on (Insert Latest Fedora Version Here).\n
A full system upgrade is recommended if before installing new dependencies\n
Please confirm that you wish to preceed by entering y below.\n\n"
read -p "Do you wish to proceed? y/N " ANSWER
ANSWER=$(printf "$ANSWER"|tr '[:upper:]' '[:lower:]')
if [ "$ANSWER" = "y" ]; then
  
  sudo dnf update 
  sudo dnf install libsndfile-devel pulseaudio-libs-devel cmake git ncurses ncurses-devel gcc wget pavucontrol gcc-c++ codec2-devel

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