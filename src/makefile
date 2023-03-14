CC = gcc
CFLAGS = -W -Wall -O3
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
TARGET = libatfs.a
INCLUDE = -I../include

CFLAGS += -D_ATFS_DEBUG
CFLAGS += -D_ATFS_LOG

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(AR) rscv $@ $^

clean:
	$(RM) $(TARGET) $(OBJECTS)