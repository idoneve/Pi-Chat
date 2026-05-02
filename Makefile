# C Version
CSTD ?= c17

# SCP target variables
USER ?=
HOST ?=
DIR ?=
REMOTE_PATH ?=

# File names
MAKENAME ?= Makefile
PROJNAME ?= $(notdir $(CURDIR))
README ?= $(wildcard README.*)

# Variables
FSTYLE ?= LLVM
OPTIMIZER ?= O2
ARGS ?=

ifeq ($(shell uname -s),Darwin)
  CC = clang
  opts = -D_FORTIFY_SOURCE=2 -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
         -Wall -Wextra -Wpedantic -Wshadow -Winit-self -Wpointer-arith \
         -Wcast-qual
else
  CC = gcc
  opts = -D_FORTIFY_SOURCE=2 -fno-diagnostics-show-option \
         -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
         -Walloc-zero -Wpedantic -Wduplicated-cond \
         -Wduplicated-branches -Wextra -Winit-self \
         -Wshadow -Wunused-const-variable=1 -Wlogical-op \
         -Wpointer-arith -Wcast-qual -Wconversion \
         -Wstrict-prototypes -Wmissing-prototypes \
         -Wsign-conversion -Wdouble-promotion
# For kernel code
# opts += -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone
endif

# Flags
CFLAGS ?= -Wall -ggdb3 -pthread -MMD -MP $(opts)

# Targets
TARGET ?= a.out
SRCS = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)
-include $(DEPS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format:
	find . \( -name '*.c' -o -name '*.h' \) | xargs clang-format -i --style=$(FSTYLE)
	@echo "Formatting complete"

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

clean_all: clean
	@echo "WARNING: WILL REMOVE dist directory and ANY .zip or tar.gz files in 3 seconds"
	@sleep 3
	rm -rf dist *.zip *.tar.gz
	@echo "All files removed"

NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
parallel_build:
	$(MAKE) -j$(NPROC) all
	@echo "Parallel build complete"

raylib_build:
	$(CC) -I/opt/homebrew/include -L/opt/homebrew/lib $(SRCS) \
	-lraylib -framework OpenGL \
	-framework Cocoa \
    -framework IOKit \
    -framework CoreVideo

debug_build: clean
	$(MAKE) CFLAGS="-Wall -ggdb3 -fsanitize=undefined,address $(opts)" $(TARGET)
	@echo "Debug build complete"

prod_build: clean
	$(MAKE) CFLAGS="-Wall -Werror $(opts)" $(TARGET)
	@echo "Production build complete"

run:
	./$(TARGET) $(ARGS)

benchmark: clean
	$(MAKE) CFLAGS="$(filter-out -g, $(opts))" $(TARGET)
	@echo "Running benchmark with -$(OPTIMIZER)..."
	@echo "=== Timing Results ==="
	@bash -c 'time ./$(TARGET) $(ARGS) > /dev/null 2>&1'
	@echo ""
	@echo "======================"
	@echo "Completed benchmark."

dist: clean
	mkdir -p dist
	tar czf dist/$(PROJNAME)_$(shell date +%Y%m%d).tar.gz *.c *.h $(README) $(MAKENAME)
	@echo "$(PROJNAME)_$(shell date +%Y%m%d).tar.gz created in dist directory."

zip: clean
	mkdir -p dist
	zip dist/$(PROJNAME).zip $(HEADERS) $(SRCS) $(MAKENAME) $(README)
	@echo "$(PROJNAME).zip created in dist directory."

scp:
	scp $(SRCS) $(HEADERS) $(MAKENAME) $(USER)@$(HOST):$(REMOTE_PATH)

docker:
	docker run --platform linux/amd64 -tiv "$(CURDIR):/valgrind" karek/valgrind:latest

memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) $(ARGS)

gdb:
	gdb $(TARGET)

info:
	@echo "Machine information:"
	@echo "    Name: $(shell uname -n)"
	@echo "    Architecture: $(shell uname -m)"
	@echo "    OS: $(shell uname -v)" 
	@echo "    CPU Cores: $(NPROC)"
	@echo ""
	@echo "Build Information:"
	@echo "    Project name: $(PROJNAME)"
	@echo "    Path: $(shell pwd)"
	@echo "    Compiler: $(CC)"
	@echo "    Version: $(shell $(CC) --version | head -1)"
	@echo "    Target file: $(TARGET)"
	@echo "    Source files: $(SRCS)"
	@echo "    Object files: $(OBJS)"
	@echo "    Docker available: $(shell which docker >/dev/null 2>&1 && echo Yes || echo No)"
	@echo ""
	@echo "Vars:"
	@echo "    CSTD=$(CSTD)"
	@echo "    MAKENAME=$(MAKENAME)"
	@echo "    README=$(README)"
	@echo "    DIR=$(DIR)"
	@echo "    USER=$(USER)"
	@echo "    REMOTE_PATH=$(REMOTE_PATH)"
	@echo "    HOST=$(HOST)"
	@echo "    OPTIMIZER=$(OPTIMIZER)"
	@echo "    ARGS=$(ARGS)"
	@echo ""
	@echo "Flags: $(CFLAGS)"

help:
	@echo "Available Make Commands:"
	@echo ""
	@echo "  make all ARGS=                          - Build the target executable"
	@echo "  make format FSTYLE=                     - Format all source code (must not be in docker)"
	@echo "  make build                              - Clean and build (alias for clean_all + all)"
	@echo "  make parallel_build                     - Build using all CPU cores"
	@echo "  make prod_build                         - Production build with -Werror"
	@echo "  make debug_build                        - Debug build with sanitizers: address and undefined (must not be in docker)"
	@echo "  make raylib_build                       - Build with raylib library for graphics"
	@echo "  make run ARGS=                          - Run the executable"
	@echo "  make benchmark ARGS= OPTIMIZER=         - Runs benchmark with optimizer (O0-O3, default O2)"
	@echo "  make scp HOST= DIR=                     - Send files to remote server"
	@echo "  make docker                             - Run interactive docker container with linux with valgrind on x86_64"
	@echo "  make memcheck ARGS=                     - Run valgrind memory checker (must be in docker)"
	@echo "  make gdb                                - Run gdb debugger (must be in docker)"
	@echo "  make zip PROJNAME= MAKENAME= README=    - Create $(PROJNAME).zip"
	@echo "  make dist [same as zip]                 - Create a timestamped tarball"
	@echo "  make clean                              - Removes program artifacts"
	@echo "  make clean_all                          - Removes everything including dist directory and any .zip or .tar.gz files"
	@echo "  make info                               - Show machine and build information, vars, and flags (vars and flags can be changed in make call)"
	@echo "  make help                               - Display this help message"

.PHONY: all build parallel_build debug_build raylib_build prod_build benchmark clean \
	    clean_all format run dist zip scp docker memcheck gdb info help