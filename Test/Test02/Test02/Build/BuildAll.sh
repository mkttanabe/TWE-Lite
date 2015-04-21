#!/bin/sh

make TWE_CHIP_MODEL=JN5148 clean
make TWE_CHIP_MODEL=JN5148 all -j 4
make TWE_CHIP_MODEL=JN5164 clean
make TWE_CHIP_MODEL=JN5164 all -j 4
