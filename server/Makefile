# C Version
CSTD ?= c23

# File names
MAKENAME ?= Makefile
README ?= $(wildcard README.*)

# Variables
OPTIMIZER ?= O2
ARGS ?=

# Flags
ifeq ($(shell uname -s),Darwin)
  CC = clang
  CFLAGS = -D_FORTIFY_SOURCE=2 -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
         -Wall -Wextra -Wpedantic -Wshadow -Winit-self -Wpointer-arith \
         -Wcast-qual -D_POSIX_C_SOURCE=199309L
else
  CC = gcc
  CFLAGS = -D_FORTIFY_SOURCE=2 -fno-diagnostics-show-option \
         -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
         -Walloc-zero -Wpedantic -Wduplicated-cond \
         -Wduplicated-branches -Wextra -Winit-self \
         -Wshadow -Wunused-const-variable=1 -Wlogical-op \
         -Wpointer-arith -Wcast-qual -Wconversion \
         -Wstrict-prototypes -Wmissing-prototypes \
         -Wsign-conversion -Wdouble-promotion -D_POSIX_C_SOURCE=199309L
endif

# Targets
SERVER_TARGET = bin/server
IP_ADDRESS ?= 127.0.0.1

# Source files
SERVER_SRC = src/server.c

# Headers
LOCAL_INCLUDE = ./include/
SHARED_INCLUDE = ../include/

all: $(SERVER_TARGET) 

bin/server: $(SERVER_SRC) $(SERVER_HDR) | bin
	$(CC) $(CFLAGS) -I$(LOCAL_INCLUDE) -I$(SHARED_INCLUDE) -o $@ $(SERVER_SRC)

bin:
	mkdir -p bin

run_server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

clean:
	rm -rf bin

.PHONY: all run_server clean
