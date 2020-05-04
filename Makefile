# This is used to have percentage in build print messages 
# overengineered, but it's nice
ifndef ECHO
T := $(shell $(MAKE) $(MAKECMDGOALS) --no-print-directory \
      -nrRf $(firstword $(MAKEFILE_LIST)) \
      ECHO="COUNTTHIS" | grep -c "COUNTTHIS")

N := x
C = $(words $N)$(eval N := x $N)
ECHO = echo -e "`expr " [\`expr $C '*' 100 / $T\`" : '.*\(....\)$$'`%]"
endif

# Defining compiler, flags, object files & socked dirs and useful variables
CC = gcc
CFLAGS = -Wall -pedantic
ODIR = objs
SDIR = sockets
GREEN = \033[0;32m
NC = \033[0m

# List all the files needed for each target
_SUPER_OBJS = supermercato/main.o supermercato/logger.o
_DIREC_OBJS = direttore/main.o
# Generate final list with object dir
DIREC_OBJS = $(patsubst %,$(ODIR)/%,$(_DIREC_OBJS))
SUPER_OBJS = $(patsubst %,$(ODIR)/%,$(_SUPER_OBJS))

# List all the header files
SUPER_HEADERS = src/supermercato/logger.h
DIREC_HEADERS =

.PHONY: all clean

all: direttore supermercato
	@[ -d $(SDIR) ] || mkdir -p $(SDIR) ]

# Delete intermediate files and others
clean:
	@echo "Deleting objs folder"
	@rm -rf objs
	@echo "Deleting output files"
	@rm *.out
	@echo "Deleting socket files"
	@rm -rf sockets

# supermercato and direttore targets
supermercato: $(SUPER_OBJS)
	@$(ECHO) "$(GREEN)Generating executable $@.out$(NC)"
	@$(CC) $(CFLAGS) $(SUPER_OBJS) $(SUPER_HEADERS) -o $@.out
	@$(ECHO) "$(GREEN)\033[1mTarget $@ built$(NC)"

direttore: $(DIREC_OBJS)
	@$(ECHO) "$(GREEN)Generating executable $@.out$(NC)"
	@$(CC) $(CFLAGS) $(DIREC_OBJS) $(DIREC_HEADERS) -o $@.out
	@$(ECHO) "$(GREEN)\033[1mTarget $@ built$(NC)"

# Object files for each source needed
$(ODIR)/supermercato/%.o: src/supermercato/%.c config.h
	@[ -d $(ODIR)/supermercato ] || mkdir -p $(ODIR)/supermercato
	@$(CC) $(CFLAGS) -c -o $@ $<
	@$(ECHO) "$(GREEN)Generating object file $@$(NC)"

$(ODIR)/direttore/%.o: src/direttore/%.c config.h
	@[ -d $(ODIR)/direttore ] || mkdir -p $(ODIR)/direttore
	@$(CC) $(CFLAGS) -c -o $@ $<
	@$(ECHO) "$(GREEN)Generating object file $@$(NC)"

# Tests
.PHONY: test1 test2

test1: all

test2: all
