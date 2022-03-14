CC=$(CROSS_COMPILE)gcc

CFLAGS=-Werror -Wall -g

LDFLAG = -pthread -lrt 

#Source
SRCS = aesdsocket.c


OBJS = $(SRCS:.c=.o)

# Target
TARGET = aesdsocket

all: $(TARGET)
default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAG) $^ -o $@
	
.PHONY: clean

clean:
	$(RM) $(TARGET) *.o
