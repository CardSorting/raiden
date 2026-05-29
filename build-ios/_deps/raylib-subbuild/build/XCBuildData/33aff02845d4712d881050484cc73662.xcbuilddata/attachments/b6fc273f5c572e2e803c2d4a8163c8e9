#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/bozoegg/Desktop/raiden/build-ios/_deps
  /opt/homebrew/bin/cmake -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/raylib-populate-prefix/tmp/raylib-populate-gitclone.cmake
  /opt/homebrew/bin/cmake -E touch /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/raylib-populate-download
fi

