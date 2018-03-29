CXX=g++
#CXXFLAGS=-Wall -DDEBUG -std=c++11
LIBRARIES    = -lblackadder -lpthread -lcrypto -lboost_system
INCLUDEB      = -I /usr/local/include
CXXFLAGS     =  -O2 -D_GNU_SOURCE -DWITH_POSIX -std=c++11
OBJ = coaphandler.o coap.o main.o logger.o subscriptions.o clients.o utils.o coap_proxy.o 

TARGET = pointcoap

default: $(TARGET)

$(TARGET) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LIBS) $(INCLUDE) $(LIBRARIES) -o pointcoap

main.o: main.c 
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
coap.o: coap.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
logger.o: logger.c
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
subscriptions.o: subscriptions.c
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
clients.o: clients.c
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
utils.o: utils.c
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
coap_proxy.o: coap_proxy.c 
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE) $(LIBRARIES)
coaphandler.o: coaphandler.cc 
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(INCLUDE) $(LIBRARIES)


clean:
	rm -f $(OBJ) $(TARGET) 


