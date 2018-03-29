all: deploy igraph_version.hpp client

deploy: igraph_version.hpp
	$(CXX) $(CXXFLAGS) bitvector.cpp graph_representation.cpp network.cpp parser.cpp deploy.cpp deployment_server.cpp odl_configuration.cpp -o deploy $(LDFLAGS) -lconfig++ -ligraph -lboost_system -lboost_thread -lpthread -lboost_filesystem
		
client: deployment_client.cpp
	$(CXX) $(CXXFLAGS) -O3 $^ -o $@ -lboost_system -lboost_thread -lpthread
	
# igraph has many problems as API changes from version to version
# this provides mechanism to define version and use #defines to
# make appropriate changes at compile time.
# do make clean if igraph is upgraded to a new version
igraph_version: igraph_version.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)
igraph_version.hpp: igraph_version
	./igraph_version > igraph_version.hpp

clean:
	rm -f deploy igraph_version.hpp igraph_version client
