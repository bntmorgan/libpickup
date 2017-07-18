#!/bin/bash

ret(){
  if test $1 -ne 0
  then
    echo User has to reauthenticate, please use : xml user auth
    exit 1
  fi
}

echo User credentials
xml user
ret $?

nbrecs=`xml recs | wc -l`

if test $nbrecs -eq 0
then
  echo scanning new recommendations...
  xml recs scan
  ret $?
else
  echo already have $nbrecs to swipe
fi

recs=`xml recs | cut -f 1 -d " "`

for i in $recs
do
  echo $i
  echo Liking...
  xml r like $i
done
