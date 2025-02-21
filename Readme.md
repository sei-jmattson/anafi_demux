# anafi_demux
A Parrot Anafi demuxer

## Usage
This demuxer transforms a Parrot Anafi USA/GOV/MIL stream (or mp4) into a
custom binary structure for each downsampled frame.

The executable dependencies are not statically linked, so the `LD_LIBRARY_PATH`
environment variable should be set to the app folder.

Examples:
```bash
$ anafi_demux
usage: anafi_demux <url> <output_path> [fps]
        url is rtsp:// or path/to/recorded.mp4
        output_path "-" means stdout, otherwise a folder for frame files
        fps defaults to 1

# connect to skycontroller and downsample to 2 fps
$ LD_LIBRARY_PATH=/opt/anafi /opt/anafi/anafi_demux rtsp://192.168.53.1/live - 2
```

## Output

| length   | description
| -------- | -------
| 2 bytes  | magic number (/x50/x41)
| 2 bytes  | version /x00/x00
| 4 bytes  | protobuf data length
| variable | protobuf data
| 2 bytes  | resolution width
| 2 bytes  | resolution height
| 4 bytes  | yuv image length
| variable | yuv image

To process the protobuf metadata, Parrot's vmeta proto specification can be found here:
https://github.com/Parrot-Developers/libvideo-metadata/blob/master/proto/vmeta.proto

## Build / Install

A x86_64 build (Ubuntu 22.04) is maintained with this repo's releases.

```bash
curl https://github.com/sei-jmattson/releases/latest/anafi_demux-linux_x86_64.tar
```

Building the app has been encapsulated in `build.sh`. That script ensures dependencies, downloads project sources, builds them, and creates a tar file of the resulting application.

The tar contains an `anafi` folder, so it can be installed by extracting to the desired destination.

```
sudo tar xf groundsdk/anafi_demux.tar -C /opt
```

Alternatively, a `Dockerfile` exists to build a docker image, if that is a convenient workflow.

```bash
docker build -t anafi_demux .
```

## TODO
- statically link dependencies to eliminate need to set LD_LIBRARY_PATH
- headless version (remove dependency on libgl1)
