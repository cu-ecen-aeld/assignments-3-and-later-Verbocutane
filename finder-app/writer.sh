#!/bin/bash
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

writefile=$1
writestr=$2

if [ $# -lt 2 ]
then
	echo "Invalid number of argument"
	exit 1
fi

OutputDir=$(dirname $writefile)

if [ ! -d $OutputDir ]
then
	mkdir -p $OutputDir;
fi

echo $writestr > $writefile
