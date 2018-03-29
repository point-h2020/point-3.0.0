#  
#  Add your example to the EXEC list
#  

CXX?=g++
LIBDIR:=../../lib/qos
CXXFLAGS?=-I$(LIBDIR) -Wall -rdynamic
LIBS:= -lblackadder -lpthread -lssl -lcrypto -lmoly -lmapi

EXEC=	linkstate_monitor \
		qos_publisher \
		qos_subscriber \
		metadata_provider \
		ping_publisher \
		ping_subscriber\
		ping_publisher_mc\
		ping_subscriber_mc\

.PHONY: all clean

all: $(EXEC)

LIBOBJS:= bytearray.o metadatapacket.o
$(LIBOBJS): %.o: $(LIBDIR)/%.cpp $(LIBDIR)/%.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generic target compile. Override if you want different behavior.
%: %.cpp $(LIBOBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

qos_%: qos_%.cpp $(LIBOBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	-rm -f $(EXEC)
	-rm -f *.o
