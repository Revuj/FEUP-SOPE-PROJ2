OBJECTS = server.o options.o queue.o log.o
ONAME = server

all: $(OBJECTS)
	$(CC) -pthread $(WARNS) $(INCLUDES) -o $(ONAME) $(OBJECTS) $(LDFLAGS)
	@rm -f $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(WARNS) -c $*.c $(INCLUDES)

clean:
	@rm -f $(OBJECTS) $(ONAME)