CC=gcc
CFLAGS=-O2 -Wall -Wextra -pthread
SRC=src/main.c src/sim.c src/parser.c
BIN=doom_sim

all: $(BIN)
$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(BIN)
