# This is used to have percentage in build print messages 
# overengineered, but it's nice
ifndef ECHO
T := $(shell $(MAKE) $(MAKECMDGOALS) --no-print-directory \
      -nrRf $(firstword $(MAKEFILE_LIST)) \
      ECHO="COUNTTHIS" | grep -c "COUNTTHIS")

N := x
C = $(words $N)$(eval N := x $N)
ECHO = echo -e "`expr "  [\`expr $C '*' 100 / $T\`" : '.*\(....\)$$'`%]"
endif

# Defining compiler, flags, object files & socked dirs and useful variables
SHELL = bash
CC = gcc
CFLAGS = -Wall -pedantic -pthread 
ODIR = objs
SDIR = sockets
GREEN = \033[0;32m
NC = \033[0m

# List all the files needed for each target
_OBJS = counter.o customer.o guard.o logger.o main.o manager.o supermarket.o utils/config.o utils/network.o utils/time.o
_LLDS_OBJS = llds/hashmap.o llds/queue.o llds/read_write_lock.o
# Generate final list with object dir
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
LLDS_OBJS = $(patsubst %,$(ODIR)/%,$(_LLDS_OBJS))

# List all the header files
HEADERS = src/counter.h src/customer.h src/guard.h src/logger.h src/manager.h src/supermarket.h src/utils.h \
		  src/utils/config.h src/utils/consts.h src/utils/network.h src/utils/time.h
LLDS_HEADERS = src/llds/errors.h src/llds/hashmap.h src/llds/queue.h src/llds/read_write_lock.h

.PHONY: all clean

all: llds supermercato 

# Delete intermediate files and others
clean:
	@echo "Deleting objs folder"
	@rm -rf $(ODIR)
	@echo "Deleting output files"
	@rm -rf *.out
	@echo "Deleting socket files"
	@rm -rf $(SDIR)
	@echo "Deleting libraries files"
	@rm -rf *.a
	@echo "Deleting valgrind files"
	@rm -rf vgcore*
	@echo "Deleting log files"
	@rm -rf test.log exec?.log

# supermercato target
supermercato: $(OBJS) llds
	@$(ECHO) "$(GREEN)Generating executable $@.out$(NC)"
	@$(CC) $(CFLAGS) $(OBJS) -o $@.out -g -L . -lllds
	@$(ECHO) "$(GREEN)\033[1mTarget $@ built$(NC)"

# Object files for each source needed
$(ODIR)/%.o: src/%.c $(HEADERS) $(LLDS_HEADERS) 
	@[ -d $(ODIR)/utils ] || mkdir -p $(ODIR)/utils
	@$(CC) $(CFLAGS) -c -o $@ $<
	@$(ECHO) "$(GREEN)Generating object file $@$(NC)"

llds: $(LLDS_OBJS)
	@$(ECHO) "$(GREEN)Generating library lib$@$(NC)"
	@ar r libllds.a $(LLDS_OBJS)
	@$(ECHO) "$(GREEN)\033[1mTarget lib$@ built$(NC)"

$(ODIR)/llds/%.o: src/llds/%.c $(LLDS_HEADERS)
	@[ -d $(ODIR)/llds ] || mkdir -p $(ODIR)/llds
	@$(CC) $(CFLAGS) -c -o $@ $<
	@$(ECHO) "$(GREEN)Generating object file $@$(NC)"
	
lldstest.out: llds src/llds/test/test.c
	@$(CC) $(CFLAGS) -g src/llds/test/test.c -L . -lllds -o lldstest.out


# Tests
.PHONY: test test1 test2 lldstest

test: all
	@valgrind --leak-check=full --trace-children=yes --show-leak-kinds=all --track-origins=yes ./supermercato.out

test1: all
	@valgrind --leak-check=full --trace-children=yes --show-leak-kinds=all --track-origins=yes ./supermercato.out config1.txt & \
	pid=$$(pgrep memcheck | head -1); \
	sleep 15; \
	kill -3 $$pid; \
	wait $$pid

test2: all
	@valgrind --leak-check=full --trace-children=yes --show-leak-kinds=all --track-origins=yes ./supermercato.out config2.txt & \
	pid=$$(pgrep memcheck | head -1); \
	sleep 25; \
	kill -1 $$pid; \
	wait $$pid; \
	./analisi.sh exec2.log

lldstest: lldstest.out
	valgrind --leak-check=full ./lldstest.out	
