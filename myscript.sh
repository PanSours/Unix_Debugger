#!/bin/bash

echo "picodb is going to trace readfile"
echo

input1=$1 #contains the commands for picodb
input2=$2

./picodb readfile< $input1 > output1.txt #run picodb with input from file in variable input1 and write the output to output1.txt
./picodb readfile< $input2 > output2.txt

echo "done tracing, info is in output1.txt and output2.txt"
echo

open=`grep -c "Found System Call 5 " output1.txt` #store number of occurances to count this type of syscall
fopen=`grep -c "Found System Call 5 " output2.txt`
close=`grep -c "Found System Call 6 " output1.txt`
fclose=`grep -c "Found System Call 6 " output2.txt`
read=`grep -c "Found System Call 3 " output1.txt`
fread=`grep -c "Found System Call 3 " output2.txt`

let gain1=(open-fopen)*100/open
let gain2=(close-fclose)*100/close
let gain3=(read-fread)*100/read

echo "---------------------------RESULTS---------------------------"
echo "         "read method    fread method    gain%
echo open"               "$open"	       "$fopen"      "$gain1

echo close"              "$close"           "$fclose"      "$gain2

echo read"           "$read"           "$fread"     "$gain3
echo "-------------------------------------------------------------"
echo

ls -l /etc/services > numberfile.txt

read temp< numberfile.txt
numberfile=${temp:23:5} #this stores the number of characters in etc/services

echo "Number of characters in /etc/services"
echo $numberfile" characters"
echo
echo "Number of characters with read method"
grep -A 0  READ output1.txt #print the line that contains the number of characters red by READ method
echo

echo "Number of characters with fread method"
grep -A 0  FREAD output2.txt
echo

echo "Number of system calls traced  with read method"
grep -A 0  "I Traced" output1.txt #print the line that contains the number of total sys calls
echo

echo "Number of system calls traced  with fread method"
grep -A 0  "I Traced" output2.txt
echo

exit
