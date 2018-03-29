CXXFLAGS = -std=c++11 -g -Wall -fmessage-length=0

OBJS =	subscriber.o

LIBS = 	-lbampers -lblackadder -lboost_system

TARGET = subscriber

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
