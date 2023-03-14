CC = gcc
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
TARGET = atfs_example
LIBS = -latfs -lpthread
LIBPATH = -L../../src
INCLUDE = -I../../include
CFLAGS = -g $(INCLUDE)

$(TARGET):$(OBJECTS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS) $(LIBPATH)

clean:
	rm -f $(TARGET) $(OBJECTS)