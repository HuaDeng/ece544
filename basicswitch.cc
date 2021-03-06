#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "basicswitch.hh" 
#include "packets.hh"

CLICK_DECLS 

BasicSwitch::BasicSwitch() {
	_num_port = 1;
}

BasicSwitch::~BasicSwitch(){
	
}

int BasicSwitch::initialize(ErrorHandler *errh){
    return 0;
}

int BasicSwitch::configure(Vector<String> &conf, ErrorHandler *errh){
	if(cp_va_kparse(conf, this, errh,
			"NUMBER_PORT", cpkP+cpkM, cpInteger, &_num_port,
                  	cpEnd) < 0) {
    	return -1;
  }
  return 0;
}


void BasicSwitch::push(int port, Packet *packet) {
	assert(packet);
	struct PacketHeader *header = (struct PacketHeader *)packet->data();
	int out_port = header->port;
	output(out_port).push(packet);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicSwitch)
