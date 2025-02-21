##
## build image
##
FROM ubuntu:jammy as build
LABEL MAINTAINER=jmattson@sei.cmu.edu

# install dependencies
RUN apt-get update && apt-get -y install \
    curl \
    git \
    python3 \
    zlib1g-dev \
    libglfw3-dev \
    libsdl2-dev \
    cmake \
    qtbase5-dev \
    build-essential \
    rsync \
    repo

# build scripts prevent building as root
RUN useradd builder -m
USER builder
WORKDIR /app

# pull source and accept EULA
RUN repo init -u https://github.com/Parrot-Developers/groundsdk-tools-manifest && \
    repo sync && \
    rm .repo/manifests/EULA*

# copy in package
COPY ./anafi_demux packages/anafi_demux

# build package
RUN ./build.sh -p groundsdk-linux -t build -A anafi_demux -j/1

##
## production image
##
FROM ubuntu:jammy
WORKDIR /opt/anafi
# needs libgl1 until we get a headless demuxer
RUN apt-get update && apt-get -y install libgl1 && rm -rf /var/lib/apt/lists/*
COPY --from=build /app/out/groundsdk-linux/final/usr/lib/lib* .
COPY --from=build /app/out/groundsdk-linux/final/usr/bin/anafi_demux .
COPY Readme.md .
ENV LD_LIBRARY_PATH=/opt/anafi
ENTRYPOINT ["/opt/anafi/anafi_demux"]
