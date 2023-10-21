Gnash MiyooCFW/OpenDingux/GKD/Funkey/ port
-

This is a trimmed down port of Gnash for OpenDingux compatible devices.
Some ports will need a boost folder relative to the Makefile.
However, it only needs the headers.

(Previously you would need to compile boost program options but that dependency has been removed)

Here are the list of dependencies required for this port of Gnash :
- libc
- an STL C++ library
- SDL 1.2 (Although the codebase supports fbdev, i would recommend you avoid that if possible)

## Installation
*Note: in these instructions, it is presumed you use the [MiyooCFW](https://github.com/TriForceX/MiyooCFW/) custom firmware on your device.*
1. Download the latest release ZIP-file or IPK-package over at [Releases](https://github.com/Apaczer/gnash-miyoo/releases/latest).
2. **ZIP:** Extract the `gnash*.zip` content to ``$HOME`` directory on your device  

	**IPK:** Launch ``gnash.ipk`` from GMenu2X's Explorer.

## Compatibility
Supports only ActionScript 2.0 games and below possibly

## Build instructions
### > PC
To build on Linux machine you should have these dependencies installed:
`libgif-dev libswscale-dev libavformat-dev libavcodec-dev ffmpeg`
### > MiyooCFW
For MiyooCFW build use static musl buildroot configuration from `bittboy_2020_static-musl` tree:
```
git clone https://github.com/MiyooCFW/buildroot
cd buildroot
git checkout bittboy_2020_static-musl
sudo make sdk
cd output/images
gzip -d arm-miyoo-linux-musleabi_sdk-buildroot.tar.gz
tar xvf arm-miyoo-linux-musleabi_sdk-buildroot.tar
mv arm-miyoo-linux-musleabi_sdk-buildroot miyoo
sudo cp -a miyoo /opt/
```
then run Makefile.miyoo inside git repository to build `gnash` binary:
```
make -f Makeile.miyoo
```
append `ipk` or `zip` option to build distribution packages.

### > PGO
It is recommended that you profile your final build before release for max performance, use these variables for that:
 - `PROFILE=YES` to generate *.gcda files at runtime
 - `PROFILE=APPLY` to apply generated profile at compile time

## Notes
For more optimization you can try to comprese & reduce accuracy of graphics by converting \*.swf with Flash Optimizer (nonfree commercial)