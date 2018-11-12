#!/bin/bash
docker build . -f Dockerfile.server -t gjz010/quagga-server
docker build . -f Dockerfile.student -t gjz010/quagga-student
