#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild
  /opt/homebrew/bin/cmake -Dcfgdir=/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME -P /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/raylib-populate-prefix/tmp/raylib-populate-mkdirs.cmake
  /opt/homebrew/bin/cmake -E touch /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/raylib-populate-mkdir
fi

