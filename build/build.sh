#!/bin/bash -eux
PICO_SDK_PATH=../3rdparty/pico-sdk cmake -DPICO_BOARD=snowdive_btm_palmtop_v0 -DCMAKE_BUILD_TYPE=Debug ..; make
