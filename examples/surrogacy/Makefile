CXXFLAGS = -std=c++11 -g -Wall -fno-strict-aliasing -fmessage-length=0

OBJS =	surrogateagent.o \

LIBS = -lboost_program_options -lboost_system  -lnet

TARGET = surrogateagent

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
