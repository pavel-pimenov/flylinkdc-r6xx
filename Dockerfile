FROM ubuntu:26.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake ninja-build make mingw-w64 \
    build-essential perl python3 pkg-config autoconf automake libtool \
    git patch unzip xz-utils bison flex nasm shellcheck \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

CMD ["bash", "build-win.sh"]
