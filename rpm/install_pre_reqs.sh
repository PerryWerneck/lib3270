#!/bin/bash

myDIR=$(dirname $(readlink -f ${0}))

grep -i "^BuildRequires:" $myDIR/*.spec \
	| cut -d: -f2 \
	| cut -d'>' -f1 \
	| cut -d= -f1 \
	| sudo xargs zypper in



