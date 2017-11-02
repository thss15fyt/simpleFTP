@CFLAGS = -Wall -fPIC
EXE = server
CC = gcc
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all:depend $(EXE)
depend:
	@$(CC) -MM $(SRC) > .depend
-include .depend
$(EXE):$(OBJ)
	@$(CC) $(OBJ) -o $(EXE)
clean:
	@rm $(EXE) $(OBJ) .depend -f
