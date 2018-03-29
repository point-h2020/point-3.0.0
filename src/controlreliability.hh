#ifndef CLICK_CONTROL_REL_HH
#define CLICK_CONTROL_REL_HH

#include "globalconf.hh"

#include <click/etheraddress.hh>
#include <clicknet/udp.h>
#include <click/timer.hh>

#define CPR_LRU_SIZE 100 // in packets
#define CPR_STATE_SIZE 100 // number of stored ACK IDs
#define CPR_MAX_RETRANSMISSIONS 3 // in packets
#define CPR_TIMER_EXPIRATION 1000 // in ms


CLICK_DECLS

class Pending_LRU_Object {
public:
    Pending_LRU_Object(const unsigned & _ackID,  const unsigned char* _data, const unsigned len  ){
		prev = NULL; next = NULL; ackID = _ackID; 
		data = (unsigned char*)malloc(len);
		memcpy(data, _data, len);
		data_len = len;
		retransmissions = 0;
	}
	
    ~Pending_LRU_Object(){
		if (data!=NULL) free (data);
	}
  
    Pending_LRU_Object* prev;
    Pending_LRU_Object* next;
    unsigned ackID;    
    unsigned char* data;
    unsigned data_len;
    unsigned char retransmissions;
};

class Pending_LRU {
public:
    Pending_LRU(){
        head = NULL; tail = NULL;
        max_num_of_pending_requests = CPR_LRU_SIZE;
    }
    ~Pending_LRU(){
		//nothing
		}
    
    HashTable <const unsigned, Pending_LRU_Object*> objects;
    HashTable <const unsigned, double> sent_times;
    Pending_LRU_Object* head;
    Pending_LRU_Object* tail;
    
    bool insert(Pending_LRU_Object* _obj, const unsigned & _ackID);
    bool update(const unsigned & _ackID); // sets entry at LRU's head
    bool remove(const unsigned & _ackID); 
    
    unsigned max_num_of_pending_requests;
    
};
/**@brief (Blackadder Core) The ControlReliability Element. */
class ControlReliability : public Element {
public:
    /**
     * @brief Constructor: it does nothing - as Click suggests
     * @return 
     */
    ControlReliability();
    /**
     * @brief Destructor: it does nothing - as Click suggests
     * @return 
     */
    ~ControlReliability();
    /**
     * @brief the class name - required by Click
     * @return 
     */
    const char *class_name() const {return "ControlReliability";}
    /**
     * @brief the port count - required by Click - it can have multiple output ports that are connected with Click "network" Elements, like ToDevice and raw sockets. 
     * It can have multiple input ports from multiple "network" devices, like FromDevice or raw sockets.
     * @return 
     */
    const char *port_count() const {return "-/-";}
    /**
     * @brief a PUSH Element.
     * @return PUSH
     */
    const char *processing() const {return PUSH;}
    /**
     * @brief Element configuration. Forwarder needs a pointer to the GlovalConf Element so that it can read the Global Configuration.
     * Then, there is the number of (LIPSIN) links. 
     * For each such link the Forwarder reads the outgoing port (to a "network" Element), the source and destination Ethernet or IP addresses (depending on the network mode) as well as the Link identifier (FID_LEN size see helper.hh).
     */
    int configure(Vector<String>&, ErrorHandler*);
    /**@brief This Element must be configured AFTER the GlobalConf Element
     * @return the correct number so that it is configured afterwards
     */
    int configure_phase() const{return 200;}
    /**
     * @brief This method is called by Click when the Element is about to be initialized. There is nothing that needs initialization though.
     * @param errh
     * @return 
     */
    int initialize(ErrorHandler *errh);
    /**@brief Cleanups everything. 
     * 
     * If stage >= CLEANUP_CONFIGURED (i.e. the Element was configured), Forwarder will delete all stored ForwardingEntry.
     */
    void cleanup(CleanupStage stage);
    /**@brief This method is called whenever a packet is received from the network (and pushed to the Forwarder by a "network" Element) or whenever the LocalProxy pushes a packet to the Forwarder.
     * 
     * LocalProxy pushes packets to the 0 port of the Forwarder. The first FID_LEN bytes are the forwarding identifier assigned by the LocalProxy.
     * The Forwarder checks with all its ForwardingEntry by oring the LIPSIN identifier with each Link identifier and pushes the packet to the right "network" elements. 
     * NOTE that the Forwarder will not push back a packet to the LocalProxy if the LIPSIN identifier matches the internal link identifier. This has to be checked before pushing the packet.
     * 
     * When a packet is pushed by the network the Forwarder checks with all its entries as well as the internal Link identifier and pushes the packet accordingly.
     * 
     * In general, if now entries that match are found the packet is killed. Moreover the packet is copied only as required by the number of entries that match the LIPSIN identifier.
     * @param port the port from which the packet was pushed. 0 for LocalProxy, >0 for network elements
     * @param p a pointer to the packet
     */
    void push(int port, Packet *p);
    
    static double getCurrentTimeInMS();

    
private:    
	/**@brief checks whether an ACK ID is already stored, thus implying a request retransmission 
    */
	bool isAckIDStored(const unsigned& _ack);
	/**@brief method to be executed upon a control plance timer expiration
    */
	void run_timer(Timer *timer);
	Timer _timer;
	
	unsigned* lastReceivedAckIDs;
    unsigned receivedAckCnt;

    Pending_LRU LRU;
    /**@brief A pointer to the GlobalConf Element for reading some global node configuration.
     */
    GlobalConf *gc;
    
    unsigned max_stored_ackIds;
	unsigned timer_expiration_ms;
	char max_retrans;
    bool byPassMode;

};

CLICK_ENDDECLS
#endif
