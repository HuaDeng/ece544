#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "basicclassifier.hh" 
#include "packets.hh"

CLICK_DECLS 

BasicClassifier::BasicClassifier() {
}

BasicClassifier::~BasicClassifier(){
	
}

int BasicClassifier::initialize(ErrorHandler *errh){
    return 0;
}

void BasicClassifier::push(int port, Packet *packet) {
	assert(packet);
	struct PacketHeader *header = (struct PacketHeader *)packet->data();
	header->port = port;
	if(header->type == 1 || header->type == 3) { //hello packet
		output(0).push(packet);
	} else if(header->type == 2) { //update packet
		output(1).push(packet);
	} else if(header->type == 4) { //data packet
		output(2).push(packet);
	} else if(header->type == 3){ //ack packet
		//output(3).push(packet);
		//click_chatter("ACK received do something...");	
	} else {		
		click_chatter("Wrong packet type");
		packet->kill();
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicClassifier)
