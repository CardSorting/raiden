#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild
  /opt/homebrew/bin/cmake -E make_directory /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/CMakeFiles/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME
  /opt/homebrew/bin/cmake -E touch /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/CMakeFiles/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/raylib-populate-complete
  /opt/homebrew/bin/cmake -E touch /Users/bozoegg/Desktop/raiden/build-ios/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/raylib-populate-done
fi

