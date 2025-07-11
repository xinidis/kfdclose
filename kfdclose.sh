#!/bin/bash

if test -z ${2}; then
    echo Usage: ${0##*/} pid fd_to_close
    exit 1
fi

sudo insmod kfdclose.ko pid=${1} fd_to_close=${2}
sudo rmmod kfdclose
