CXXFLAGS = -std=c++11 -g -w -fmessage-length=0

BOOTSTRAPPINGOBJS =	configuration.o \
		discovery.o \
		attachment.o \
		bootstrapping.o

LIBS = 	-lblackadder \
		-lpthread \
		-lboost_system \
		-lboost_thread 
		
BOOTSTRAPPING = bootstrapping

$(BOOTSTRAPPING):	$(BOOTSTRAPPINGOBJS)
	$(CXX) -o $(BOOTSTRAPPING) $(BOOTSTRAPPINGOBJS) $(LIBS)
	

all: $(BOOTSTRAPPING)

clean:
	rm -f $(BOOTSTRAPPINGOBJS) $(BOOTSTRAPPING)