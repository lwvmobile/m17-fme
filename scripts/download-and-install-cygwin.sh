#! /bin/bash
#
cdir=$(pwd)
clear
printf "M17 Project: Florida Man Edition - Auto Installer For Cygwin.
This will install the required and recommended packages, clone, build, and install M17-FME.
This has been tested on the latest Cygwin64 installation as of 2025-09-07.
A full system upgrade is recommended if before installing new dependencies
Please confirm that you wish to preceed by entering y below.\n\n"
read -p "Do you wish to proceed? y/N " ANSWER
ANSWER=$(printf "$ANSWER"|tr '[:upper:]' '[:lower:]')
if [ "$ANSWER" = "y" ]; then

  #is this needed?
  LD_LIBRARY_PATH=/usr/local/lib
  export LD_LIBRARY_PATH
  echo $LD_LIBRARY_PATH

  #CODEC2
  cd $cdir
  printf "Installing codec2\n Please wait!\n"
  git clone https://github.com/drowe67/codec2.git
  cd codec2
  mkdir build
  cd build
  cmake ..
  make -j $(nproc)
  make install

#   RTL-SDR #leave here in case this is added in later on
#   cd $cdir
#   printf "Installing RTL-SDR\n Please wait!\n"
#   git clone https://github.com/lwvmobile/rtl-sdr.git
#   cd rtl-sdr
#   mkdir build
#   cd build
#   cmake ..
#   make -j $(nproc)
#   make install

  #M17-FME
  cd $cdir
  printf "Installing M17-FME\n Please wait!\n"
  git clone --recursive https://github.com/lwvmobile/m17-fme.git
  cd m17-fme
  mkdir build
  cd build
  cmake ..
  make -j $(nproc)
  make install
  cd $cdir

  #call the cyg_portable script
  #sh m17-fme/cyg_portable.sh #TODO: Add this for M17-FME

  printf "Any issues, Please report to:\nhttps://github.com/lwvmobile/m17-fme/issues \n\n"

else
  printf "Thank you, have a nice day!\n\n"
fi