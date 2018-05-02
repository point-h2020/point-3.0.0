INC_DIR = .
#-DDMALLOC -DDMALLOC_FUNC_CHECK if dmalloc should be included
#-DTRAFFIC_CONTROL if traffic control should be enabled
CXXFLAGS = -std=c++11 -O2 -g -Wall -fno-strict-aliasing \
			-fmessage-length=0 -I$(INC_DIR)

OBJS =	api/napsa.o \
		configuration.o \
		demux/demux.o \
		icncore/icn.o \
		main.o \
		monitoring/collector.o \
		monitoring/statistics.o \
		namespaces/namespaces.o \
		namespaces/ip.o \
		namespaces/mcast.o \
		namespaces/http.o \
		namespaces/management.o \
		namespaces/management/dnslocal.o \
		namespaces/buffercleaners/ipbuffercleaner.o \
		namespaces/buffercleaners/httpbuffercleaner.o \
		proxies/http/dnsresolutions.o \
		proxies/http/httpproxy.o \
		proxies/http/headerparser.o \
		proxies/http/surrogatetcpclient.o \
		proxies/http/tcpclient.o \
		proxies/http/tcpclienthelper.o \
		proxies/http/tcpclientread.o \
		proxies/http/tcpserver.o \
		sockets/ipsocket.o \
		trafficcontrol/trafficcontrol.o \
		trafficcontrol/dropping.o \
		transport/transport.o \
		transport/lightweight.o \
		transport/lightweighttimeout.o \
		transport/buffercleaners/ltpbuffercleaner.o \
		transport/buffercleaners/ltpreversecidcleaner.o \
		transport/unreliable.o \
		types/eui48.o \
		types/icnid.o \
		types/ipaddress.o \
		types/netmask.o \
		types/nodeid.o \
		types/routingprefix.o

LIBS =	-lblackadder \
		-lboost_program_options \
		-lboost_thread \
		-lboost_system \
		-lconfig++ \
		-llog4cxx \
		-lmapi \
		-lmoly \
		-lnet \
		-lpcap \
		-lpthread

TARGET = nap

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	
install:
	cp $(TARGET) /usr/bin
	mkdir -p /etc/nap
	mkdir -p /usr/share/doc/nap
	cp doc/nap.cfg /usr/share/doc/nap
	cp doc/nap.l4j /usr/share/doc/nap
	cp README.md /usr/share/doc/nap/README
	cp -aui doc/nap.cfg /etc/nap/
	cp -aui doc/nap.l4j /etc/nap/
	
uninstall:
	rm /usr/bin/$(TARGET)
	rm -rf /etc/nap
	rm -rf /usr/share/doc/nap
	rm /var/log/nap.log*
