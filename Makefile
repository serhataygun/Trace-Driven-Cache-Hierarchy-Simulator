CC = gcc
CFLAGS = -O2 -Wall -Wextra

SRCS = main.c parser.c engine.c analysis.c
OBJS = $(SRCS:.c=.o)
EXEC = cache_sim

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c cache.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
