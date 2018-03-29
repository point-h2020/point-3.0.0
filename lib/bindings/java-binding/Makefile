CXX=g++
JNI_LOC = ${JAVA_HOME}/include
#JNI_LOC = /usr/lib/jvm/java-6-sun/include
JNI_INC+=-I$(JNI_LOC) -I$(JNI_LOC)/linux

NAME=eu_pursuit_client_BlackadderWrapper


all: $(NAME).o $(NAME).so

$(NAME).o: src/main/c++/$(NAME).cc
	$(CXX) -c -g -fPIC $(CXXFLAGS) $(JNI_INC) -o /tmp/$(NAME).o src/main/c++/$(NAME).cc

$(NAME).so: /tmp/$(NAME).o
	$(CXX) -shared -fPIC -o /tmp/$(NAME).so /tmp/$(NAME).o $(LDFLAGS) -lblackadder

clean:
	rm -f /tmp/$(NAME).o /tmp/$(NAME).so
