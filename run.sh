#/bin/bash

SERVER_NAME="server"
CLIENT_NAME="client"
BUILD_DIR="build"
DEPENDENCIES_DIR="common"
cflags="-Wall -O3"
dependencies=(
  "${DEPENDENCIES_DIR}/logger.c"
  "${DEPENDENCIES_DIR}/byte.c"
  "${DEPENDENCIES_DIR}/string.c"
  "${DEPENDENCIES_DIR}/signal.c"
  "${DEPENDENCIES_DIR}/fd.c"
  "${DEPENDENCIES_DIR}/socket.c"
  "${DEPENDENCIES_DIR}/connection.c"
)

if [[ "$1" == "$SERVER_NAME" ]]; then
  dependencies+=(
    "${DEPENDENCIES_DIR}/thread.c"
  )
elif [[ "$1" != "$CLIENT_NAME" ]]; then
  exit 1
fi

if [[ "$2" == "-p" || "$2" == "--production" ]]; then
  cflags="${cflags} -D PRODUCTION"
fi

mkdir -p $BUILD_DIR                                                \
  && gcc $cflags ${dependencies[@]} "$1/$1.c" -o "${BUILD_DIR}/$1" \
  && ${BUILD_DIR}/$1
