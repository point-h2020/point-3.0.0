CXXFLAGS = -std=c++11 -g -w -fmessage-length=0

SERVEROBJS =	../../deployment/bitvector.o \
		../../deployment/graph_representation.o \
		../../deployment/network.o \
		../../deployment/odl_configuration.o \
		messages/protobuf_rw.o \
		messages/messages.pb.o \
		resource_manager.o \
		tm_resource.o \
		server.o

LIBS = 	-lblackadder \
		-ligraph \
		-lboost_system \
		-lboost_thread \
		-lprotobuf \
		-lpthread \
		-lmoly
		
SERVER = server

$(SERVER):	$(SERVEROBJS)
	$(CXX) -o $(SERVER) $(SERVEROBJS) $(LIBS)
	

all: $(SERVER)

clean:
	rm -f $(SERVEROBJS) $(SERVER)