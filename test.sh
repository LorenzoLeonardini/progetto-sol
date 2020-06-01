#!/bin/bash

mkdir tmp

counter=0
groups=0
signal=1

while true; do

	time=$RANDOM
	let "time %= 15"
	let "time += 5"
	valgrind --leak-check=full --trace-children=yes --show-leak-kinds=all --track-origins=yes --log-file="tmp/log.txt" ./supermercato.out config1.txt & \
	pid=$(pgrep memcheck | head -1); \
	sleep $time; \
	kill -$signal $pid; \
	wait $pid

	len=$(cat tmp/log.txt | wc -l)
	if [[ $len -ne 24 ]]; then
		file=$(date +%d-%m-%Y-%H.%M.%s.txt)
		telegram-notify "Error in project test with signal $signal. Saved as $file" 2
		mv tmp/log.txt "tmp/$file"
		./kill.sh
	else
		let "counter += 1"
		if [[ $counter = 50 ]]; then
			let "groups += 1"
			telegram-notify "$(($counter * $groups)) successful tests for project" 3
			counter=0
		fi
	fi
	let "signal += 2"
	let "signal %= 4"

done;
