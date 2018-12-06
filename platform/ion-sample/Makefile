TARGET = ion_mem_map
SRCS = osal.c clstlib.c ion_mem_map.c ion_mem.c

CFLAGS =
INCLUDES = -I/usr/include -I.
LDFLAGS = -lOpenCL -lm -lpthread

CC = gcc
RM = rm -f

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET)

