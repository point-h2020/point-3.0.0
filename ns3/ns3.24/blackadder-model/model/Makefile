
# This Makefile provides a mechanism to define igraph version and use #defines to
# make appropriate changes at compile time.
# do make clean if igraph is upgraded to a new version
igraph_version: igraph_version.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

igraph_version.h: igraph_version
	./igraph_version > igraph_version.h

clean:
	rm igraph_version igraph_version.h
