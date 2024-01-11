#!/bin/sh
git pull
mkdir build_directory
cd build_directory
cmake ../
make