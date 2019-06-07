FROM ubuntu:disco

RUN apt-get update \
  && apt-get install --no-install-recommends -y \
  build-essential \
  autoconf \
  libtool \
  pkg-config \
  git \
  cmake \
  golang-go \
  && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["./build_and_run_tests.sh"]
