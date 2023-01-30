#!/bin/bash
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

filesdir=$1
searchstr=$2

if [ $# -lt 2 ]
then
	echo "Invalid number of argument"
elif [ -d $filesdir ]
then
	X=$(ls $filesdir | wc -l)
	Y=$(grep $searchstr $filesdir/*  | wc -l)
	echo "The number of files are ${X} and the number of matching lines are ${Y}"
else
	echo "${filesdir} is not a directory"
fi

