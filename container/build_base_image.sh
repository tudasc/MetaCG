#!/usr/bin/env bash

# Building the container base image is not possible on Lichtenberg due to a permission issue
# with Podman. Thus, we build a base image on another machine, which is then downloaded and used
# by the CI

docker login registry.git.rwth-aachen.de
docker build -t registry.git.rwth-aachen.de/tuda-sc/projects/metacg/base:latest -f container/base .
docker push registry.git.rwth-aachen.de/tuda-sc/projects/metacg/base:latest
