CXXFLAGS = -g -Wall -fmessage-length=0 -I include -std=c++11
#-DRV_MOLY_DEBUG

LIBS = 	-lmoly -lboost_system -lboost_thread -lssh

rv_to_moly:	rv_to_moly.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

clean:
	rm rv_to_moly

