#! /bin/bash
#
clear
printf "Project M17 - Florida Man Edition
Automatic Git Pull and Rebuild\n\n"
sleep 1
##Open your clone folder##
git pull
sleep 2
##cd into your build folder##
cd build
cmake ..
make -j $(nproc)
sudo make install
sudo ldconfig