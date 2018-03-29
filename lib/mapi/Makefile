# Makefile template for shared library

INCLUDE_DIR=/usr/local/include
LIB_DIR=/usr/local/lib

CXX = g++ # CPP compiler
CXXFLAGS = -fPIC -Wall -Wextra -O2 -g # CPP flags
LDFLAGS = -shared  -rdynamic# linking flags
RM = rm -f  # rm command
TARGET_LIB = libmapi.so # target lib

HDRS = mapi.hpp mapiagent.hpp
SRCS = mapi.cpp mapiagent.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CXX) ${LDFLAGS} -o $@ $^

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CXX) $(CXXFLAGS) -MM $< >$@

-include $(SRCS:.cpp=.d)

install: $(HDRS)
	mkdir -p $(INCLUDE_DIR)
	cp $(TARGET_LIB) $(LIB_DIR)
	cp $(HDRS) $(INCLUDE_DIR)
	ldconfig

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.cpp=.d)
uninstall:
	make clean
	${RM} $(LIB_DIR)/$(TARGET_LIB)
	cd $(INCLUDE_DIR); ${RM}$(HDRS)
	ldconfig
