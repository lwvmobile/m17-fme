#! /bin/bash
#
cdir=$(pwd)
clear
printf "M17 Project: Florida Man Edition - Auto Installer For Red Hat, Fedora, RHEL based Distros.
This will install the required and recommended packages, clone, build, and install M17-FME.
This has been tested on Fedora Workstation 40.
A full system upgrade is recommended if before installing new dependencies
Please confirm that you wish to preceed by entering y below.\n\n"
read -p "Do you wish to proceed? y/N " ANSWER
ANSWER=$(printf "$ANSWER"|tr '[:upper:]' '[:lower:]')
if [ "$ANSWER" = "y" ]; then
  
  sudo dnf update 
  sudo dnf install libsndfile-devel pulseaudio-libs-devel pulseaudio-utils cmake git ncurses ncurses-devel gcc wget gcc-c++ codec2-devel

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