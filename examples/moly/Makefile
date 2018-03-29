CXXFLAGS = -g -Wall -fmessage-length=0

LIBS = 	-lmoly -lboost_system -lboost_thread -lssh

ALL =	cpuutilisation \
		process

all: $(ALL)

cpuutilisation:	cpuutilisation.cc
	$(CXX) $^ -o $@ $(LIBS) ../../apps/nap/types/ipaddress.o

process:	process.cc
	$(CXX) $^ -o $@ $(LIBS)
	
clean:
	rm -f $(ALL)
