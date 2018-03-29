INCLUDE_DIR=/usr/local/include/moly
PRIMITIVES_DIR=/usr/local/include/moly/primitives
LIB_DIR=/usr/local/lib

LIBS = -lboost_system -lboost_thread 

CXXFLAGS = -fPIC -g -c -Wall 
#-DMOLY_DEBUG #-DMONITORING_TRIGGER_BUGFIX

PRIMITIVES = primitives/addlink.hh \
			primitives/addnode.hh \
			primitives/addport.hh \
			primitives/buffersizes.hh \
			primitives/channelaquisitiontime.hh \
			primitives/cmcgroupsize.hh \
			primitives/cpuutilisation.hh \
			primitives/e2elatency.hh \
			primitives/filedescriptorstype.hh \
			primitives/httprequestsfqdn.hh \
			primitives/linkstate.hh \
			primitives/matchesnamespace.hh \
			primitives/networklatencyfqdn.hh \
			primitives/nodestate.hh \
			primitives/packetdroprate.hh \
			primitives/packeterrorrate.hh \
			primitives/packetjittercid.hh \
			primitives/pathcalculations.hh \
			primitives/portstate.hh \
			primitives/publishersnamespace.hh \
			primitives/rxbyteshttp.hh \
			primitives/rxbytesip.hh \
			primitives/rxbytesipmulticast.hh \
			primitives/rxbytesport.hh \
			primitives/subscribersnamespace.hh \
			primitives/txbyteshttp.hh \
			primitives/txbytesip.hh \
			primitives/txbytesipmulticast.hh \
			primitives/txbytesport.hh
			
HEADERS = 	agent.hh \
			enum.hh \
			moly.hh \
			molyhelper.hh \
			primitives.hh \
			process.hh \
			typedef.hh

OBJS =	agent.o \
		moly.o \
		molyhelper.o \
		primitives/addlink.o \
		primitives/addnode.o \
		primitives/addport.o \
		primitives/buffersizes.o \
		primitives/channelaquisitiontime.o \
		primitives/cmcgroupsize.o \
		primitives/cpuutilisation.o \
		primitives/e2elatency.o \
		primitives/filedescriptorstype.o \
		primitives/httprequestsfqdn.o \
		primitives/linkstate.o \
		primitives/matchesnamespace.o \
		primitives/networklatencyfqdn.o \
		primitives/nodestate.o \
		primitives/packetdroprate.o \
		primitives/packeterrorrate.o \
		primitives/packetjittercid.o \
		primitives/pathcalculations.o \
		primitives/portstate.o \
		primitives/publishersnamespace.o \
		primitives/rxbyteshttp.o \
		primitives/rxbytesip.o \
		primitives/rxbytesipmulticast.o \
		primitives/rxbytesport.o \
		primitives/subscribersnamespace.o \
		primitives/txbyteshttp.o \
		primitives/txbytesip.o \
		primitives/txbytesipmulticast.o \
		primitives/txbytesport.o \
		process.o

TARGET = libmoly.so

$(TARGET):	$(OBJS)
	$(CXX) -shared -rdynamic -Wl,-soname,$(TARGET) -o $(TARGET) $(OBJS) -lc $(LIBS)

install: $(PRIMITIVES) $(HEADERS)
	mkdir -p $(INCLUDE_DIR)
	mkdir -p $(PRIMITIVES_DIR)
	cp $(TARGET) $(LIB_DIR)
	cp $(HEADERS) $(INCLUDE_DIR)
	cp $(PRIMITIVES) $(PRIMITIVES_DIR)
	ldconfig
	
clean:
	rm -f $(OBJS) $(TARGET)

uninstall:
	make clean
	rm -f $(LIB_DIR)/$(TARGET)
	cd $(INCLUDE_DIR) && rm -f $(HEADERS)
	cd $(PRIMITIVES_DIR) && cd ../ && rm -f $(PRIMITIVES) && rm -rf primitives
	rm -rf $(INCLUDE_DIR)
	rm -rf $(PRIMITIVES_DIR)
	ldconfig
