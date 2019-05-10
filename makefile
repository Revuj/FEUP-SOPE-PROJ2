  
.PHONY: all clean server user

# gcc compiler
CC = gcc

CFLAGS = -pthread -O3 -march=native -lm

LDFLAGS = -lm -pthread -lrt

# include only headers in the same directory
INCLUDES = -I ./

# -Wno-unused-result: let writes fail silently.
# -Wno-unused-parameter: the signal handlers don't use their parameter.
WARNS = -Wall -Wextra -Wno-unused-result -Wno-unused-parameter

export CC INCLUDES CFLAGS LDFLAGS WARNS

all: server user

server:
	@$(MAKE) -C src/server
	mv src/server/server .

user:
	@$(MAKE) -C src/user
	mv src/user/user .

clean:
	@$(MAKE) clean -C src/server
	@$(MAKE) clean -C src/user
	rm -f server user