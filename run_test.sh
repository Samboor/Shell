#!/bin/sh

cd shell
make full_clean
make
cd ../tests
./clean.sh
./prepare.sh
./run_suite.sh ../shell/bin/mshell 4