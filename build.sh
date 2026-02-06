#!/bin/bash

docker build \
    -t esp32orm_zephyr \
    -f Dockerfile \
    .

ssh-keygen -f "${HOME}/.ssh/known_hosts" -R '[localhost]:2223'
