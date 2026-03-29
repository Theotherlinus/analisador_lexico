CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -I.
EXEC = main

$(EXEC): main.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean run

clean:
	rm -f $(EXEC) main.o *~ core

run: $(EXEC)
	./$(EXEC)
