#!/bin/bash

while true; do
	sleep 60
	echo "Checking...."
	now=$(date +%s)
	edit=$(stat -c '%Y' tmp/log.txt)
	diff=$(($now - $edit))
	if [[ $diff -ge 180 ]]; then
		echo "STUCK"
		telegram-notify "Program seems to got stucked" 2
		break
	fi
	echo "Program is fine"
done
