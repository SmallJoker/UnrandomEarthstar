#!/bin/bash

make

if [ $? -ne 0 ]; then
	exit
fi

./UnrandomEarthstar "$@"
