#!/bin/bash
##
## unattended build script for anafi_demux
## jmattson@sei.cmu.edu
##

set -e
dist=dist/anafi
out_tar=anafi_demux.tar

## setup
sudo apt-get update
sudo apt-get install curl git python3 zlib1g-dev libglfw3-dev libsdl2-dev cmake qtbase5-dev build-essential rsync repo
mkdir -p ./groundsdk
cd ./groundsdk

## download
repo init -u https://github.com/Parrot-Developers/groundsdk-tools-manifest
repo sync
if [ -f ".repo/manifests/EULA.md" ]; then
    rm .repo/manifests/EULA* 2> /dev/null
fi

## build
cp -r ../anafi_demux ./packages
./build.sh -p groundsdk-linux -t build -A anafi_demux -j/1

# package
mkdir -p $dist
cp ../Readme.md $dist
cp out/groundsdk-linux/final/usr/bin/anafi_demux $dist
cp -d out/groundsdk-linux/final/usr/lib/lib* $dist
tar cf $out_tar -C dist .

# install
# sudo tar xf $out_tar -C /opt
