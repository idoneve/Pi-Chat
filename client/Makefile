# C Version
CSTD ?= c11

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

CLIENT_TARGET = bin/client
IP_ADDRESS ?= 127.0.0.1

CLIENT_SRC = src/client.c

# Headers
LOCAL_INCLUDE = ./include/
SHARED_INCLUDE = ./../include/

all: $(CLIENT_TARGET)

bin/client: $(CLIENT_SRC) | bin
	$(CC) $(CFLAGS) -I$(LOCAL_INCLUDE) -I$(SHARED_INCLUDE) -o $@ $(CLIENT_SRC)

bin:
	mkdir -p bin

run_client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET) $(IP_ADDRESS)

clean:
	rm -rf bin

.PHONY: all run_client clean
