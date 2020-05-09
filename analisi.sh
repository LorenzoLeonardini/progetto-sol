#!/bin/bash
## Script to collect data from a log file and display it into tabular format

# Files
if [[ $# == 1 ]]; then
	readonly LOG_FILE=$1
else
	readonly LOG_FILE="esempio.log"
fi
readonly CLIENT_FILE="/tmp/sol_clienti"
readonly COUNTER_FILE="/tmp/sol_casse"

# Formatting codes
readonly BOLD="\033[1m"
readonly CLEAR="\033[0m"
readonly RED="\033[31m"
readonly GREEN="\033[32m"

# variables used to store data and/or to cache data before printing
mode=0 # 0: boh - 1: general - 2: clients - 3: counters
incrementing='' # used to store which data to increment in counter dict
signal='????'
n_clients=0
n_products=0

# dictionaries for client and counter objects values
declare -A client=(\
	["id"]=-1 \
	["n_prod"]=0 \
	["t_tot"]=0 \
	["t_que"]=0 \
	["n_que"]=0 \
)
declare -A counter=(\
	["id"]=-1 \
	["n_prod"]=0 \
	["n_client"]=0 \
	["t_tot"]=0 \
	["t_avg"]=0 \
	["n_close"]=0 \
)

print_header() {
	local dash_len=$((48 - ${#LOG_FILE} / 2)) # calculate ---- length
	printf "%0.s-" $(seq 1 $dash_len) # print it
	printf " ${BOLD}Analisi del file di log $LOG_FILE${CLEAR} " # print header and file name
	dash_len=$((96 - $dash_len - ${#LOG_FILE})) # calculate precise ---- length (needed for odd lengths)
	printf "%0.s-" $(seq 1 $dash_len) # print it
	echo -e "\n"
}
print_header

horizontal_line() {
	echo "  ----------------------------------------------------------------------------------------------------------------------"
}
# echo the headline into the file, this will be passed to column
print_client_header() {
	echo -e "\t id  cliente \tn. prodotti acquistati\ttempo totale nel super.\ttempo tot. speso in coda\t n. di  code visitate \t" > $CLIENT_FILE
	echo -e "\t-------------\t----------------------\t-----------------------\t------------------------\t----------------------\t" >> $CLIENT_FILE
}
print_counter_header() {
	echo -e "\tid cassa\tn. prodotti elaborati\tn. di clienti\ttempo tot. di apertura\ttempo medio di servizio\tn. di chiusure\t" > $COUNTER_FILE
	echo -e "\t--------\t---------------------\t-------------\t----------------------\t-----------------------\t--------------\t" >> $COUNTER_FILE
}

# prepare files with header
print_client_header
print_counter_header

# print formatted client data into temporary file
print_client() {
	if [[ ! ${client['id']} == -1 ]]; then
		printf "\t%s\t%d\t%.3fs\t%.3fs\t%d\t\n" \
			${client['id']} \
			${client['n_prod']} \
			${client['t_tot']} \
			${client['t_que']} \
			${client['n_que']} \
			>> $CLIENT_FILE
		client['id']=-1
	fi
}

# print formatted counter data into temporary file
print_counter() {
	if [[ ! ${counter['id']} == -1 ]]; then
		if [[ ! ${counter['n_close']} == "0" ]]; then
			counter['t_avg']=$((${counter['t_tot']} / ${counter['n_close']}))
		else
			counter['t_avg']=${counter['t_tot']}
		fi
		printf "\t%s\t%d\t%d\t%.3fs\t%.3fs\t%d\t\n" \
			${counter['id']} \
			${counter['n_prod']} \
			${counter['n_client']} \
			$(echo "scale = 3; ${counter['t_tot']} / 1000" | bc) \
			$(echo "scale = 3; ${counter['t_avg']} / 1000" | bc) \
			${counter['n_close']} \
			>> $COUNTER_FILE
		counter['id']=-1
		counter['t_tot']=0
	fi
}

# check if file exists before continuing
if [[ ! -e $LOG_FILE ]]; then
	echo -e "${RED}${BOLD}Il file $LOG_FILE non esiste${CLEAR}"
	exit -1
fi

# save default IFS and replace it to keep tabs when reading the log
old_IFS=$IFS
IFS=$'\n'

# configure bash to do case insensitive comparisons
# This gives more flexibility to the log format
shopt -s nocasematch

# Read and parse log file
while read -r line; do
	# it's a comment
	if [[ $line == \#* ]]; then
		continue
	fi
	# it's a mode change
	if [[ $line == --\ * ]]; then
		case "${line:3}" in
			"generale") mode=1;;
			"clienti") mode=2;;
			"casse") mode=3;;
		esac
	elif [[ $mode == 1 ]]; then
		# we are in general data mode, retrieve all the possible data and save it
		case "$line" in
			signal:\ *) signal=${line:8};;
			numero\ clienti:\ *) n_clients=${line:16};;
			prodotti\ venduti:\ *)n_products=${line:18};;
		esac
	elif [[ $mode == 2 ]]; then
		# client mode
		# if it doesn't start with tab, it's a new client and a new id
		if [[ ! $line == $(printf '\t')* ]]; then
			print_client
			client['id']="${line::-1}"
		else
			# every time value is converted from millis to seconds. To me, a log file with milliseconds makes more sense
			case "${line:1}" in
				tempo\ nel\ supermercato:\ *) client['t_tot']=$(echo "scale = 3; ${line:24} / 1000" | bc);;
				tempo\ in\ coda:\ *) client['t_que']=$(echo "scale = 3; ${line:15} / 1000" | bc);;
				numero\ prodotti:\ *) client['n_prod']=${line:17};;
				numero\ cambi\ coda:\ *) client['n_que']=${line:20};;
			esac
		fi
	elif [[ $mode == 3 ]]; then
		# counter mode
		# if it starts with two tabs, then it's a list
		if [[ $line == $(printf '\t\t')* ]]; then
			if [[ $incrementing != '' ]]; then
				counter[$incrementing]=$((${counter[$incrementing]} + ${line:2}))
			fi
		elif [[ $line == $(printf '\t')* ]]; then # new data
			case "${line:1}" in
				numero\ clienti:\ *) counter['n_client']=${line:17};;
				aperture:*) incrementing="";;
				numero\ chiusure:\ *) counter['n_close']=${line:18};;
				numero\ prodotti:\ *) counter['n_prod']=${line:18};;
				tempo\ clienti:*) incrementing="t_tot";;
			esac
			continue
		else # new id
			print_counter
			counter['id']="${line::-1}"
		fi
	else
		continue
	fi
done < $LOG_FILE

# restore default IFS
IFS=$old_IFS

# print final client and counter that have not been printed yes
print_client
print_counter

## print nicely formatted data
# general supermarket data
echo -e "\t${GREEN}${BOLD}Supermercato Chiuso Con Segnale${CLEAR}: ${BOLD}$signal${CLEAR}"
echo -e "\t${GREEN}${BOLD}Numero Totale Clienti${CLEAR}: ${BOLD}$n_clients${CLEAR}"
echo -e "\t${GREEN}${BOLD}Numero Totale Prodotti Venduti${CLEAR}: ${BOLD}$n_products${CLEAR}"
echo ""

# client table
horizontal_line
column -t -s $'\t' -o " | " -R 3,4,5,6 < $CLIENT_FILE
horizontal_line

echo "" # blank line

# counter table
horizontal_line
column -t -s $'\t' -o " | " -R 3,4,5,6,7 < $COUNTER_FILE
horizontal_line

# clean temporary files
rm $CLIENT_FILE $COUNTER_FILE

