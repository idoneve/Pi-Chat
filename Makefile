# Compiler
GPP = gcc

# Compilation flags
GPPFlags = -Wall -Wextra
# -Wall -Wextra -Werror -Wfatal-errors
Debug = -g

SERVER_SRC = ./server.c

HEADER = $(wildcard *.h)

SERVER_OBJ = $(SERVER_SRC:.c=.o)

TARNAME = final_submission

all: client server

client:
	cd ./client/; make;

server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_OBJ)
	$(GPP) $(GPPFlags) $(SERVER_OBJ) -o $(SERVER_TARGET) 

# Compile
%.o: %.c
	$(GPP) $(GPPFlags) -c $< -o $@

.PHONY: clean package client

# clean
clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Removed all object files."

package:
	zip $(TARNAME).zip Makefile $(SRC) $(HEADER)
