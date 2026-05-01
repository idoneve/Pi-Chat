# Compiler
GPP = gcc

# Compilation flags
GPPFlags = -Wall -Wextra -Werror -Wfatal-errors
Debug = -g

CLIENT_SRC = ./client.c
SERVER_SRC = ./server.c

HEADER = $(wildcard *.h)

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

TARNAME = final_submission

# Target Name
SERVER_TARGET = ./server
CLIENT_TARGET = ./client

all: client server clean

client: $(CLIENT_OBJ)
	$(GPP) $(GPPFlags) $(CLIENT_OBJ) -o $(CLIENT_TARGET) 
server: $(SERVER_OBJ)
	$(GPP) $(GPPFlags) $(SERVER_OBJ) -o $(SERVER_TARGET) 

# Compile
%.o: %.c
	$(GPP) $(GPPFlags) -c $< -o $@

.PHONY: clean package

# clean
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)
	@echo "Removed all object files."

package:
	zip $(TARNAME).zip Makefile $(SRC) $(HEADER)
