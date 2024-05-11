#! /bin/bash
#
cdir=$(pwd)
clear
printf "Project M17: Florida Man Edition - Auto Installer For Arch Linux Based Distros\n
This will install the required and recommended packages, clone, build, and install M17-FME\n.
This has been tested on Arch 2023.08.01 and Manjaro XFCE 22.1.3 Minimal.\n
A full system upgrade is recommended if before installing new dependencies\n
Please confirm that you wish to preceed by entering y below.\n\n"
read -p "Do you wish to proceed? y/N " ANSWER
ANSWER=$(printf "$ANSWER"|tr '[:upper:]' '[:lower:]')
if [ "$ANSWER" = "y" ]; then
  #always run a full update first, partial upgrades aren't supported in Arch 
  #including downloading dependencies, that may require an updated dependency, 
  #and breaks same dependency on another package, a.k.a, dependency hell
  sudo pacman -Syu 
  sudo pacman -S libpulse cmake ncurses codec2 base-devel libsndfile git wget

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