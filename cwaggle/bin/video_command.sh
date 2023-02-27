#!/bin/bash
ffmpeg -r 96 -i ../../screenshots/%d.png -pix_fmt yuv420p $1
