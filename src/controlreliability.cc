#include "controlreliability.hh"

CLICK_DECLS

ControlReliability::ControlReliability()  : _timer(this)  {
}

ControlReliability::~ControlReliability() {
	click_chatter("ControlReliability: destroyed!");
}

int ControlReliability::configure(Vector<String> &conf, ErrorHandler */*errh*/) {
	gc = (GlobalConf *) cp_element(conf[0], this);
	if (conf.size()>1 && strcmp(conf[1].c_str(), "true")==0){
		byPassMode = false;
		}
	else byPassMode = true;
	
	click_chatter("****************************ControlReliability CONFIGURATION**************************");
	click_chatter("Bypass Mode: %d", (unsigned) byPassMode);
	if (!byPassMode) click_chatter("Timer expiration: %d (ms), Max retransmissions: %d, Stored ACK-IDs: %d, LRU size: %d (pcks)", 
		(unsigned) CPR_TIMER_EXPIRATION, CPR_MAX_RETRANSMISSIONS, CPR_STATE_SIZE, CPR_LRU_SIZE);

	click_chatter("*******************************************************************************");

	return 0;
}

int ControlReliability::initialize(ErrorHandler */*errh*/) {
	//click_chatter("Forwarder: Initialized!");
    receivedAckCnt=0;
    _timer.initialize(this);   // Initialize timer object (mandatory)
    
    max_stored_ackIds = CPR_STATE_SIZE;
    lastReceivedAckIDs = (unsigned*) malloc (sizeof(unsigned) * max_stored_ackIds);
	timer_expiration_ms = CPR_TIMER_EXPIRATION; 
    max_retrans = CPR_MAX_RETRANSMISSIONS;
    srand(atoi(gc->nodeID.c_str()));
    
	return 0;
}

void ControlReliability::cleanup(CleanupStage stage) {
	if (stage >= CLEANUP_CONFIGURED) {
	      free (lastReceivedAckIDs);
	  }
	click_chatter("ControlReliability: Cleaned Up!");
}
/*
 * Searches stored ackIDs (in order to avoid pushing a retransmitted request to localProxy)
 * Could use a different structure fi. hashmap to allow faster retrievals; O(1) instead of O(n)
 * */
bool ControlReliability::isAckIDStored(const unsigned& _ack){
    for (unsigned i=0;i<max_stored_ackIds;i++){
        if (lastReceivedAckIDs[i] == _ack)
            return true;
        }
    return false;
    }

void ControlReliability::push(int in_port, Packet *p) {
	WritablePacket *newPacket = NULL;
	WritablePacket *payload = NULL;
	unsigned char type;
	unsigned char tm_numberOftm_IDs, tm_IDLength;
	String tm_ID;
	int tm_id_index;
	int ackID;

	/*0 port comes from proxy and goes to forwarder*/
	if (in_port == 0) {
		//click_chatter("CPR: got packet from proxy");
		/* 1st check if byPassMode is enabled
		 * 2nd parse packet's FID, RID and type
		 * If its not from TM neither to RV then discard
		 * else insert CPR header and create pending_request state
		 * */
		 /// byPass on -> transparent operation
	      if (byPassMode){
			output(0).push(p);
			return; 
		  }
	      //incoming structure:: || FID | numOfIDs | IDLen | ID | TYPE | PAYLOAD
	      BABitvector forwardFID(FID_LEN * 8);
	      memcpy(forwardFID._data, p->data(), FID_LEN); 
	      int type_pos = FID_LEN; // will be increamented until it reaches req type position 
	      unsigned char _numOfIDs = *(p->data() + type_pos); type_pos++;
	      /* control plane messages have only one RID */
	      if (_numOfIDs!=1){
		  output(0).push(p);
		  return; 
		  }
	      unsigned char _IDLen = *(p->data() + type_pos); type_pos++;
	      String _rID = String((const char *) (p->data() + type_pos), (int) _IDLen * PURSUIT_ID_LEN);
	      //click_chatter("IDlen %d %d rID %s", _IDLen, _IDLen*PURSUIT_ID_LEN, _rID.c_str());
	      type_pos += PURSUIT_ID_LEN * _IDLen;
	
		  unsigned char packet_type = *(p->data() + type_pos);
		  //click_chatter("CPR:: got packet from proxy with type %d PUBLISH_DATA %d MATCH_PUB_SUBS %d", (unsigned) packet_type, (unsigned)PUBLISH_DATA, (unsigned)MATCH_PUB_SUBS);	  
		  /* not from TM, neither to RV*/
		  if ((!gc->type[TM] || _rID.substring(0, PURSUIT_ID_LEN).compare(gc->notificationIID.substring(0,PURSUIT_ID_LEN).c_str()) != 0) && 
			(packet_type==PUBLISH_DATA || !forwardFID.to_string().equals(gc->defaultRV_dl.to_string()))){
			 // click_chatter("gc->type[TM] %d _rID %s (compare = %d)packet_type %d forwardFID %s", 				(unsigned)gc->type[TM], _rID.substring(0, PURSUIT_ID_LEN).c_str(), (unsigned) _rID.compare(gc->notificationIID.c_str(), PURSUIT_ID_LEN), (unsigned)packet_type,  forwardFID.to_string().c_str());
			  output(0).push(p);
				  return; 
		  }//else click_chatter("Got a reliable pub/sub request.");
	      //outgoing structure:: || FID | << numOfIDs | IDLen | REL_SCOPE | CNTR_REQ | reverseFID | ACKID >> | numOfIDs | IDLen |TYPE | PAYLOAD
		  /* insert control reliability header*/
		  payload = p->uniqueify(); 
		  unsigned char * tmp_buff = (unsigned char*) malloc (payload->length() + sizeof(char) /*num_of_ids*/ + sizeof(char) /*ID_length*/ + PURSUIT_ID_LEN + sizeof (type) + FID_LEN /*reverseFID*/+ sizeof (ackID));
		  int offset = 0;
		  memcpy(tmp_buff + offset, payload->data(), FID_LEN); // first add FID
		  offset += FID_LEN;
		  tmp_buff[offset++] = 1; //then add num_of_ids
		  tmp_buff[offset++] = gc->controlReliabilityScope.length() / PURSUIT_ID_LEN; //then add ID_length
		  memcpy(tmp_buff+offset, gc->controlReliabilityScope.c_str(), PURSUIT_ID_LEN); // then add control reliability ID
	      offset += PURSUIT_ID_LEN;
		  tmp_buff[offset++] = CONTROL_REC; // then add type (request)
		  if (!gc->type[TM]) // otherwise the dst node knowns the FID to send back ACK to the TM
			memcpy(tmp_buff + offset, (char*)gc->defaultFromRV_dl._data, FID_LEN); // add reverseFID 
		  offset += FID_LEN;
		  ackID = rand(); 
		  tmp_buff[offset++] = (ackID >> 24) & 0xFF; // then add ACKID
		  tmp_buff[offset++] = (ackID >> 16) & 0xFF;
		  tmp_buff[offset++] = (ackID >> 8) & 0xFF;
		  tmp_buff[offset++] = ackID & 0xFF;
		  memcpy(tmp_buff+offset, payload->data()+FID_LEN, payload->length() - FID_LEN); // then add the rest of the payload
		  newPacket = payload->put(offset);
		  memcpy(newPacket->data(), tmp_buff, newPacket->length());
	     
	      /* store pending reguest */
	      Pending_LRU_Object* obj = new Pending_LRU_Object(ackID, tmp_buff, newPacket->length());
	      LRU.insert(obj, ackID);
	   	  
	   	  output(0).push(newPacket);
		  free(tmp_buff);
		  p->kill();
		  //click_chatter("ControlReliability:: Sent CONTROL_REQ with AckID %d", ackID);
			
		  if (!_timer.scheduled())
			_timer.schedule_after_msec(timer_expiration_ms);
		} 
	/* 1 port comes from forwarder and goes to proxy */
	else if (in_port == 1) {
		/* 1st parse packet's FID and RID to check if its a CPR packet
		 * 2nd parse CPR header to get type: Request or ACK
		 * if (Request) check if request RID is notificationIID, then select the appropriate reverse FID and send ACK
		 * else remove the pending_request with the same CPR (Ack)ID. 
		 * */ 		
		 //incoming structure:: || FID | numOfIDs | IDLen | ID | TYPE | PAYLOAD
		 /* parse FID, RIDs etc */
		 //click_chatter("CPR: got packet from network");
		 int offset = 0;
		 BABitvector FID(FID_LEN * 8);
		 memcpy(FID._data, p->data(), FID_LEN); 
		 offset += FID_LEN;
		 unsigned char numberOfIDs = *(p->data() + offset); 
		 offset += sizeof(numberOfIDs);
		 if (numberOfIDs != 1){
				/* this is not a control-reliability related packet */
				p->pull(FID_LEN);
				output(1).push(p); 
				//click_chatter("ControlReliability:: Not control reliable packet, wrong numberOfIDs: %d, Pushing upwards!!", numberOfIDs);
				return;
		 } 
		 unsigned char IDLength =  *(p->data() + offset); 
		 offset+= sizeof(IDLength);
		 String _ID = String((const char*) (p->data() + offset), IDLength * PURSUIT_ID_LEN); 
		 offset+= IDLength * PURSUIT_ID_LEN;
		 if (_ID.compare(gc->controlReliabilityScope) != 0) {
				/* this is not a control-reliability related packet */
				p->pull(FID_LEN);
				output(1).push(p); 
				//click_chatter("ControlReliability:: Not control reliable packet, ID: %s. Pushing upwards!", _ID.c_str());
				return;
		 } 
		 /* parse CPR header: type, (ack)ID */
		 char type = *(p->data() + offset); 
		 int type_pos = offset;
		 offset+=sizeof(type);
		 char * reverseFID = (char*)(p->data() + offset);
		 offset += FID_LEN;
		 /* parse ACK_ID */
		 ackID = (*(p->data() + offset) << 24) | (*(p->data() + offset +1) << 16) | (*(p->data() + offset +2) << 8) | *(p->data() + offset + 3);
		 offset += 4;
		 if (type == CONTROL_REC){
				/* parse ID to see if its a TM notification, thus requiring different FID */
				tm_id_index = 0;
				tm_numberOftm_IDs = *(p->data()+offset);
				tm_IDLength = *(p->data() + offset + sizeof (tm_numberOftm_IDs) + tm_id_index);
				tm_ID = (String((const char *) (p->data() + offset + sizeof (tm_numberOftm_IDs) + sizeof (tm_IDLength) + tm_id_index), (int) tm_IDLength * PURSUIT_ID_LEN));
				tm_id_index = tm_id_index + sizeof (tm_IDLength) + tm_IDLength * PURSUIT_ID_LEN;
				/* create and send CONTROL_ACK */
				WritablePacket *pp = Packet::make(p->data(), offset);
				/* select appropriate reverse FID*/
				if (tm_ID.compare(gc->notificationIID) == 0){
					//click_chatter ("ControlReliability:: Got control message from TM");
					memcpy(pp->data(), gc->TMFID._data, FID_LEN); //update FID
				}else{
					//click_chatter ("ControlReliability:: Got control message from user");
					memcpy(pp->data(), reverseFID, FID_LEN); //update FID
				}
				pp->data()[type_pos] = CONTROL_ACK; // update request type
				output(0).push(pp);
				//click_chatter("ControlReliability:: Got CONTROL_REQ with AckID %d and sent CONTROL_ACK", ackID);
				/* forward req to local Proxy */
				if (!isAckIDStored(ackID)){
					payload = p->uniqueify();
					payload->pull(offset);
					output(1).push(payload);					
					/*store ack ID */
					lastReceivedAckIDs[receivedAckCnt++] = ackID;
					if (receivedAckCnt == max_stored_ackIds)
						receivedAckCnt = 0;
				}
		 } else if (type == CONTROL_ACK){
			  //click_chatter("ControlReliability:: Got CONTROL_ACK with AckID %d", ackID);
			  /* remove Pending_LRU entry */
			  LRU.remove(ackID);
		 }else {
			  click_chatter("ControlReliability:: Unknown type of message published under CONTROL RELIABILITY scope. Ignoring packet");
			  }
			// have to remove control reliability header
			// at this point packet structure is: " CONTR_REL_HDR | ID | PAYLOAD "
			p->kill();
	}else  {    
		click_chatter("ControlReliability:: packet from unknown port.. exiting!");
		p->kill();
	}

}

double ControlReliability::getCurrentTimeInMS() {
	struct timeval  tv;     
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond
}
	
void ControlReliability::run_timer(Timer *timer) {
	// This function is called when the timer fires.
    assert(timer == &_timer);
    /* check if a timer has expired */
    if (LRU.objects.size()==0){
		return;
	}
	/* check if a timer has expired */
    double now = getCurrentTimeInMS();
	int diff = now - LRU.sent_times[LRU.tail->ackID] - timer_expiration_ms;
    // not a timeout *yet*!
    if (diff < 0){ 
		_timer.schedule_after_msec(-diff);
		//click_chatter("reschedualed after %d ms",(unsigned)-diff);
	}else{ // time out		
		/* resent packet */
		Pending_LRU_Object *obj = LRU.objects[LRU.tail->ackID];
		// push failure delivery to the user and remove pending entry
		if (obj->retransmissions >= max_retrans) {
			/* push failure notification to locap app */
			// packet structure:: || FID | numOfIds | IdLen | RELIABILITY_SCOPE | CONTROL_REQ | revFID | ACKID | PAYLOAD ||
			// output structure::       || numOfIds | IdLen | RELIABILITY_SCOPE | CONTROL_REQ | DELIVERY_STATUS | PAYLOAD ||
			unsigned char * data = (unsigned char*) malloc(obj->data_len - 2 * FID_LEN - sizeof(int) + sizeof(char) ); // remove FID, revFID and ackID but insert status char
			unsigned int offset = sizeof(char) + /*num_of_ids*/ + sizeof(char) /*ID_length*/ + PURSUIT_ID_LEN /*ID*/ + sizeof (char) /* type*/;
			memcpy (data, obj->data + FID_LEN, offset); // copied RV_ID and type
			unsigned int offset2 = offset;
			data[offset2] = CONTROL_REQ_FAILURE; // copied status
			offset2 += sizeof(char);
			unsigned int payload_len = obj->data_len - 2*FID_LEN - offset - sizeof(int);
			memcpy (data + offset2, obj->data + 2*FID_LEN + offset + sizeof(int),  payload_len);
			WritablePacket *pp = Packet::make(data, offset2 + payload_len);
			click_chatter("ControlReliability:: Did not retransmit REQ with ackID %d (retransmission %d)", obj->ackID, obj->retransmissions);
			output(1).push(pp); // push notification to user
			/* update LRU */
			LRU.remove(obj->ackID);
			free(data);
			}
		else{
			// retransmitt request
			WritablePacket *pp = Packet::make(obj->data, obj->data_len);
			output(0).push(pp);
			// update LRU and sent times
			LRU.update(obj->ackID);
			//LRU.sent_times[obj->ackID] = ControlReliability::getCurrentTimeInMS();
			click_chatter("ControlReliability::Retransmitted REQ with ackID %d (retransmission %d)", obj->ackID, obj->retransmissions);
			obj->retransmissions++;
		}
		/* reschedule timer */
		if (LRU.objects.size()>0){
			unsigned n= (now - LRU.sent_times[LRU.tail->ackID] - timer_expiration_ms) > 1 ? (now - LRU.sent_times[LRU.tail->ackID] - timer_expiration_ms) : 1;
			_timer.schedule_after_msec(n);
		} 
	}
}


bool Pending_LRU::insert(Pending_LRU_Object* _obj, const unsigned & _ackID){
       
    // obj already stored
    if (objects.find(_ackID) != objects.end()){
        update (_ackID);
        return 1;
    }
    // ensure LRU will not grow beyond maximum size
    while (objects.size() > max_num_of_pending_requests){
		remove(tail->ackID);
	}
    objects[_ackID] = _obj; // store object 
    sent_times[_ackID] = ControlReliability::getCurrentTimeInMS(); // store time

    /* update LRU */
    if (head == NULL){ // LRU is empty
      head = _obj;
      tail = _obj;
      //click_chatter("First item in LRU");
    }else{
        head->next = _obj;
        _obj->prev = head;
        head = _obj;
        }
       //click_chatter("%lf Inserted packet %d, head %d, tail %d",sent_times[_ackID] , _ackID, head->ackID, tail->ackID);
    return 0;
}
    
bool Pending_LRU::update(const unsigned & _ackID){
    Pending_LRU_Object* obj = (objects.find(_ackID)->second);
    sent_times[_ackID] = ControlReliability::getCurrentTimeInMS();
    if (obj->ackID == head->ackID){ // its head
		//click_chatter("Its already the head of the LRU");
        return 1;
    }if (obj->ackID == tail->ackID && objects.size()!=1){ 
        tail = obj->next;
        tail->prev = NULL;
    }else { // its in the middle
        obj->next->prev = obj->prev;
        obj->prev->next = obj->next;
        }
    head->next = obj;
    obj->prev = head;
    head = obj;
   
    return 0;
}
    
bool Pending_LRU::remove(const unsigned & _ackID){
	if (objects.find(_ackID) == objects.end()){
		//item must have been removed before receiving the ACK, due to space limitations
		return 1;
		}
		
    Pending_LRU_Object* obj = (objects.find(_ackID)->second);
    if (objects.size() != 1){ // its not the only item in map
        if (obj->ackID == head->ackID){ // its head
            obj->prev->next = NULL;
            head = obj->prev;
        }else if (obj->ackID == tail->ackID){ // its tail
            tail = obj->next;
            tail->prev = NULL;
        }else { // its in the middle
            obj->next->prev = obj->prev;
            obj->prev->next = obj->next;
            }
        //click_chatter("Removed packet %d, head %d, tail %d", _ackID, head->ackID, tail->ackID);
    }  else {
        head = NULL;
        tail = NULL;
        //click_chatter("Removed packet %d, LRU is empty", _ackID);
        }
    sent_times.erase(_ackID); // remove time
    objects.erase(_ackID); // remove time
	delete obj;
    return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ControlReliability)

