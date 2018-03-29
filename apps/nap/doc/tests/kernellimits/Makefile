INC_DIR = . 

CXXFLAGS =	-std=c++11 -O0 -g -Wall -fno-strict-aliasing \
			-fmessage-length=0 -I$(INC_DIR)

OBJS =	main.o \
		tcpclient.o

LIBS =	-lpthread

TARGET = kernellimits

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
