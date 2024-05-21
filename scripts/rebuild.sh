#! /bin/bash
#
clear
printf "M17 Project - Florida Man Edition
Automatic Git Pull and Rebuild\n\n"
sleep 1
##Open your clone folder##
git pull
sleep 2
##cd into your build folder##
cd ..
cd build
cmake ..
make -j $(nproc)
sudo make install
sudo ldconfig