#!/bin/bash
## Script to collect data from a log file and display it into tabular format

# Files
if [[ $# == 1 ]]; then
	readonly LOG_FILE=$1
else
	readonly LOG_FILE="esempio.log"
fi
readonly CUSTOMER_FILE="/tmp/sol_clienti"
readonly COUNTER_FILE="/tmp/sol_casse"

exec 3>$CUSTOMER_FILE
exec 4>$COUNTER_FILE

# Formatting codes
readonly BOLD="\033[1m"
readonly CLEAR="\033[0m"
readonly RED="\033[31m"
readonly GREEN="\033[32m"

# variables used to store data and/or to cache data before printing
mode=0 # 0: boh - 1: general - 2: customers - 3: counters
incrementing='' # used to store which data to increment in counter dict
signal='????'
n_customers=0
n_products=0

# dictionaries for customer and counter objects values
declare -A customer=(\
	["id"]=-1 \
	["n_prod"]=0 \
	["t_tot"]=0 \
	["t_que"]=0 \
	["n_que"]=0 \
)
declare -A counter=(\
	["id"]=-1 \
	["n_prod"]=0 \
	["n_customer"]=0 \
	["t_tot"]=0 \
	["t_cust"]=0 \
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
print_customer_header() {
	echo -e "\t id  cliente \tn. prodotti acquistati\ttempo totale nel super.\ttempo tot. speso in coda\t n. di  code visitate \t" >&3
	echo -e "\t-------------\t----------------------\t-----------------------\t------------------------\t----------------------\t" >&3
}
print_counter_header() {
	echo -e "\tid cassa\tn. prodotti elaborati\tn. di clienti\ttempo tot. di apertura\ttempo medio di servizio\tn. di chiusure\t" >&4
	echo -e "\t--------\t---------------------\t-------------\t----------------------\t-----------------------\t--------------\t" >&4
}

# prepare files with header
print_customer_header
print_counter_header

# print formatted customer data into temporary file
print_customer() {
	if [[ ! ${customer['id']} == -1 ]]; then
		printf "\t%s\t%d\t%.3fs\t%.3fs\t%d\t\n" \
			${customer['id']} \
			${customer['n_prod']} \
			"${customer['t_tot']}e-3" \
			"${customer['t_que']}e-3" \
			${customer['n_que']} \
			>&3
		customer['id']=-1
	fi
}

# print formatted counter data into temporary file
print_counter() {
	if [[ ! ${counter['id']} == -1 ]]; then
		if [[ ! ${counter['n_close']} == "0" ]]; then
			counter['t_avg']=$((${counter['t_cust']} / ${counter['n_customer']}))
		else
			counter['t_avg']=${counter['t_cust']}
		fi
		printf "\t%s\t%d\t%d\t%.3fs\t%.3fs\t%d\t\n" \
			${counter['id']} \
			${counter['n_prod']} \
			${counter['n_customer']} \
			"${counter['t_tot']}e-3" \
			"${counter['t_avg']}e-3" \
			${counter['n_close']} \
			>&4
		counter['id']=-1
		counter['t_tot']=0
		counter['t_cust']=0
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
			numero\ clienti:\ *) n_customers=${line:16};;
			prodotti\ venduti:\ *)n_products=${line:18};;
		esac
	elif [[ $mode == 2 ]]; then
		# customer mode
		# if it doesn't start with tab, it's a new customer and a new id
		if [[ ! $line == $(printf '\t')* ]]; then
			print_customer
			customer['id']="${line::-1}"
		else
			# every time value is converted from millis to seconds. To me, a log file with milliseconds makes more sense
			case "${line:1}" in
				tempo\ nel\ supermercato:\ *) customer['t_tot']=${line:24};;
				tempo\ in\ coda:\ *) customer['t_que']=${line:15};;
				numero\ prodotti:\ *) customer['n_prod']=${line:17};;
				numero\ cambi\ coda:\ *) customer['n_que']=${line:20};;
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
				numero\ clienti:\ *) counter['n_customer']=${line:17};;
				aperture:*) incrementing="t_tot";;
				numero\ chiusure:\ *) counter['n_close']=${line:18};;
				numero\ prodotti:\ *) counter['n_prod']=${line:18};;
				tempo\ clienti:*) incrementing="t_cust";;
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

# print final customer and counter that have not been printed yes
print_customer
print_counter

## print nicely formatted data
# general supermarket data
echo -e "\t${GREEN}${BOLD}Supermercato Chiuso Con Segnale${CLEAR}: ${BOLD}$signal${CLEAR}"
echo -e "\t${GREEN}${BOLD}Numero Totale Clienti${CLEAR}: ${BOLD}$n_customers${CLEAR}"
echo -e "\t${GREEN}${BOLD}Numero Totale Prodotti Venduti${CLEAR}: ${BOLD}$n_products${CLEAR}"
echo ""

exec 3>&-
exec 4>&-

# customer table
horizontal_line
column -t -s $'\t' -o " | " -R 3,4,5,6 < $CUSTOMER_FILE
horizontal_line

echo "" # blank line

# counter table
horizontal_line
column -t -s $'\t' -o " | " -R 3,4,5,6,7 < $COUNTER_FILE
horizontal_line

# clean temporary files
rm $CUSTOMER_FILE $COUNTER_FILE

