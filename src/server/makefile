OBJECTS = server.o options.o
ONAME = server

all: $(OBJECTS)
	$(CC) $(WARNS) $(INCLUDES) -o $(ONAME) $(OBJECTS) $(LDFLAGS)
	@rm -f $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(WARNS) -c $*.c $(INCLUDES)

clean:
	@rm -f $(OBJECTS) $(ONAME)