LIBRARIES = -lcoap-1
INCLUDE = -I /usr/local/include/coap/
CXXFLAGS = -O2 -D_GNU_SOURCE -DWITH_POSIX

all: servers
	
servers: demo_server.c
	g++ demo_server.c $(INCLUDE) $(LIBRARIES) $(CXXFLAGS) -g -o demo_server



