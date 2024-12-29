#!/bin/bash
make
echo "12345" | ./reverser
./controller <testfile name>.usp <testedfile name>.usp 
# replace filename to your test file