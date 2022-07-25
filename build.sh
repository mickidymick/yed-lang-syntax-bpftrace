#!/usr/bin/env bash
gcc -o bpftrace.so bpftrace.c $(yed --print-cflags) $(yed --print-ldflags)
