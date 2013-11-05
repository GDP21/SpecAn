#!/bin/sh

for release in "debug" "test" "release"; do
  for threadID in "0" "1"; do
    for oper in "libclean" "libs"; do
      make RELEASE=$release THREAD_ID=$threadID $oper;
    done;
  done;
done;
