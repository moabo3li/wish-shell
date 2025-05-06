CC=gcc
# Add -D_GNU_SOURCE to enable POSIX features like strdup and getline
CFLAGS=-Wall -Wextra -Werror 
TARGET=wish
SRCS=wish.c
OBJS=$(SRCS:.c=.o)

# Default target - build the shell
all: $(TARGET)

# Compile the shell
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Generate object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Clean up compiled files
clean:
	rm -f $(TARGET) $(OBJS)

# Debug build with symbols
debug: CFLAGS += -g -DDEBUG
debug: clean all


.PHONY: all clean debug install help

