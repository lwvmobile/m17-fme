#! /bin/bash
#
clear
printf "M17 Project - Florida Man Edition
Automatic Git Pull and Rebuild\n\n"
sleep 1
##pull latest from git
git pull
##cd into your build folder##
cd ..
cd build
cmake ..
make -j $(nproc)
sudo make install
sudo ldconfig