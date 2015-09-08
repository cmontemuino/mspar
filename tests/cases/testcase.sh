#!/bin/bash

if [ $# -eq 0 ]
then
    echo "No test case argument supplied"
else
  testcase=$1
  params="./params.case.$testcase"
  if [ -f "$params" ]
  then
    casedir="case$testcase"
    if [ ! -d $casedir ]
    then
      mkdir $casedir
      echo "Created output folder $casedir"
      source $params
      for i in {1..7}
      do
        echo "-> Running iteration ${i} of 7 for test case $testcase"
        filename="${casedir}/case${testcase}.${i}"
        STARTTIME=$(date +%s)
        /usr/bin/time -v -o "$filename.time" mpirun -n 4 ../../bin/mspar ${cmd} > "$filename".out
        touch "${casedir}/case${testcase}.${i}.time"
        ENDTIME=$(date +%s)
        echo "--> Done in $(($ENDTIME - $STARTTIME)) seconds."
      done
      echo "Test case ${testcase} done!"
    else
      echo "Output directory $casedir already exists."
    fi
    
    
  else
    echo "File $params does not exist"
  fi  
fi
