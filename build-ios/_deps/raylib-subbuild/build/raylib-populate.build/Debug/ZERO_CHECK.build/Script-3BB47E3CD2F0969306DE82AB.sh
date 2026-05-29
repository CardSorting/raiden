#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild
  make -f /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/CMakeScripts/ReRunCMake.make
fi

