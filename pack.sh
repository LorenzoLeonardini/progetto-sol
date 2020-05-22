#!/bin/bash

rm -rf lorenzo_leonardini-CorsoA.tar.gz
cp report/report.pdf leonardini_relazione_sol.pdf
tar -czvf lorenzo_leonardini-CorsoA.tar.gz Makefile analisi.sh src config.txt config?.txt leonardini_relazione_sol.pdf
rm leonardini_relazione_sol.pdf
