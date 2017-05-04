#!/bin/bash

command -v align_image_stack >/dev/null 2>&1 || { echo >&2 "I require align_image_stack (from pano tools/hugin package) but it's not installed.  Aborting."; exit 1; }

align_image_stack -a aligned $@

