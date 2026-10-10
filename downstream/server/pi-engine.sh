#!/bin/sh
set -eu
exec /usr/local/bin/qemu-i386 -L /opt/cod2-i386 \
  /opt/cod2-i386/usr/local/bin/cod2_lnxded "$@"
