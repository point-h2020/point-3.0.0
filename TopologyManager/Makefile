CXX?=g++
LIBDIR:=../lib/qos
CXXFLAGS?=-I$(LIBDIR) -Wall -std=c++11
LIBS:=-lblackadder -lpthread -ligraph -lcrypto -lmoly -lboost_system -lboost_thread

.PHONY: all clean

all: igraph_version.hpp igraph_version tm	rm

LIBOBJS:=metadatapacket.o lsmpacket.o bytearray.o
$(LIBOBJS): %.o: $(LIBDIR)/%.cpp $(LIBDIR)/%.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp %.hpp igraph_version.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tm_graph.cpp: igraph_version.hpp

tm_igraph.cpp: igraph_version.hpp

# igraph has many problems as API changes from version to version
# this provides mechanism to define version and use #defines to
# make appropriate changes at compile time.
# do make clean if igraph is upgraded to a new version
igraph_version: igraph_version.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

igraph_version.hpp: igraph_version
	./igraph_version > igraph_version.hpp

tm: tm_graph.o tm_igraph.o tm_qos.o tm_max_flow.o te_graph_mf.o \
	$(LIBOBJS) tm.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

rm: tm_graph.o tm_igraph.o tm_max_flow.o te_graph_mf.o rm.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	-rm -f tm rm *.o igraph_version.hpp igraph_version
