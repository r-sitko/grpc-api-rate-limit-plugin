#!/usr/bin/env bash
set -ex

TESTS_DIR=$(dirname $(readlink -f $0))
cd $TESTS_DIR
GIT_REPO_ROOT=$(git rev-parse --show-toplevel)

docker build -t grpc-rate-limit-tests -f Dockerfile .
docker run --rm -v $GIT_REPO_ROOT:/grpc-rate-limit -w /grpc-rate-limit/test \
  --name grpc-rate-limit-tests grpc-rate-limit-tests:latest
