INCLUDE_DIR=/usr/local/include/bampers
TYPES_DIR=/usr/local/include/bampers/types
DATAPOINTS_DIR=/usr/local/include/bampers/datapoints
LIB_DIR=/usr/local/lib

LIBS = 	-lblackadder \
		-lboost_thread
		
CXXFLAGS = -fPIC -g -c -std=c++11 -Wall 
#-DBAMPERS_DEBUG

HEADERS = 	bampers.hh \
			cidanalyser.hh \
			icneventhandler.hh \
			enum.hh \
			namespace.hh \
			typedef.hh \

TYPES =	types/icnid.hh

DATAPOINTS =	datapoints/link.hh \
				datapoints/node.hh \
				datapoints/port.hh \
				datapoints/statistics.hh \

OBJS =	bampers.o \
		cidanalyser.o \
		datapoints/link.o \
		datapoints/node.o \
		datapoints/port.o \
		datapoints/statistics.o \
		icneventhandler.o \
		namespace.o \
		types/icnid.o

TARGET = libbampers.so

$(TARGET):	$(OBJS)
	$(CXX) -shared -rdynamic -Wl,-soname,$(TARGET) -o $(TARGET) $(OBJS) $(LIBS) -lc

install: $(HEADERS)
	mkdir -p $(INCLUDE_DIR)
	mkdir -p $(INCLUDE_DIR)/datapoints
	mkdir -p $(INCLUDE_DIR)/types
	cp $(TARGET) $(LIB_DIR)
	cp $(HEADERS) $(INCLUDE_DIR)
	cp $(TYPES) $(TYPES_DIR)
	cp $(DATAPOINTS) $(DATAPOINTS_DIR)
	ldconfig
	
clean:
	rm -f $(OBJS) $(TARGET)

uninstall:
	make clean
	rm -f $(LIB_DIR)/$(TARGET)
	rm -rf $(INCLUDE_DIR)
	ldconfig
