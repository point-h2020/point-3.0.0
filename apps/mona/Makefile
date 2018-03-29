CXXFLAGS = -std=c++11 -g -Wall -fmessage-length=0 
#-DMONA_DEBUG #-DMONITORING_TRIGGER_BUGFIX

OBJS =	main.o

LIBS = 	-lbampers\
		-lblackadder \
		-lboost_program_options \
		-lboost_system \
		-lboost_thread \
		-lmapi \
		-lmoly

TARGET = mona

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

install:
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/bin/mona
	
clean:
	rm -f $(OBJS) $(TARGET)
