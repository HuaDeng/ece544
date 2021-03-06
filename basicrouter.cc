#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include <click/hashtable.hh>
#include "basicrouter.hh" 

CLICK_DECLS 

BasicRouter::BasicRouter() : _timerHello(this), _timerPortHello(this), _timerUpdate(this), _timerPortUpdate(this){
	_seq = 0;
	_periodHello = 5;
	_periodUpdate = 5;
	_periodPort = 0.1;
	_time_out = 1;
	_my_address = 0;
	_other_address = 0;
	_num_port = 0;	
	_port_cnt_hello = 0;
	_port_cnt_update = 0;
}

BasicRouter::~BasicRouter(){
	
}

int BasicRouter::initialize(ErrorHandler *errh){
	click_chatter("--------------router initial--------------");
	_timerHello.initialize(this);
	_timerUpdate.initialize(this);
	_timerPortHello.initialize(this);
	_timerPortUpdate.initialize(this);
	_timerHello.schedule_after_sec(_periodHello);
	_timerUpdate.schedule_after_sec(_periodUpdate+8);	
	return 0;
}

int BasicRouter::configure(Vector<String> &conf, ErrorHandler *errh){
	if(cp_va_kparse(conf, this, errh,
			"MY_ADDRESS", cpkP+cpkM, cpUnsigned, &_my_address,
			"NUMBER_PORT", cpkP+cpkM, cpInteger, &_num_port,
                  	"OTHER_ADDRESS", cpkP, cpUnsigned, &_other_address,
                  	"PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
                  	"TIME_OUT", cpkP, cpUnsigned, &_time_out,
                  	cpEnd) < 0) {
    	return -1;
  }
  return 0;
}

void BasicRouter::run_timer(Timer *timer){ 
	if(timer == &_timerHello){ //periodically send hello packet
		_port_cnt_hello = 0;
		click_chatter("PERIODICAL: Sending new hello packet\n");
		WritablePacket *hello = Packet::make(0,0,sizeof(struct PacketHeader), 0);
		memset(hello->data(),0,hello->length());
		struct PacketHeader *header = (struct PacketHeader*) hello->data();
		header->type = 1;
		header->source = _my_address;
		header->sequence = _seq;
		header->port = _port_cnt_hello;
		output(0).push(hello);
		_timerPortHello.schedule_after_sec(_periodPort);
		_timerHello.schedule_after_sec(_periodHello);
	}else if(timer == &_timerUpdate){
		_port_cnt_update = 0;
		click_chatter("PERIODICAL: Sending new update packet");
		WritablePacket *update = Packet::make(0,0,sizeof(struct PacketHeader), 0);
		memset(update->data(), 0, update->length());
		struct PacketHeader *header = (struct PacketHeader*) update->data();
		header->type = 2; //update packet
		header->source = _my_address;
		header->sequence = _seq;
		header->port = _port_cnt_update;
		header->length = sizeof(_routing_table)*_routing_table.size();
		HashTable<uint16_t, struct TableEntry> *data = (HashTable<uint16_t, TableEntry> *)(update->data() + sizeof(struct PacketHeader));
		memcpy(data, &_routing_table, sizeof(_routing_table)*_routing_table.size());
		output(1).push(update);

		
		/*
		click_chatter("Router %u routing table", _my_address);
		int entry = 0;
		for(HashTable<uint16_t, struct TableEntry>::iterator _topo_table_iter = _routing_table.begin(); _topo_table_iter != _routing_table.end(); ++_topo_table_iter){
			for(int i = 0; i < _topo_table_iter.value().nextHop.size(); i++){
				click_chatter("*********************Entry %u: Dst %u Cost %d NextHop %d*********************", entry, _topo_table_iter.key(), _topo_table_iter.value().cost, _topo_table_iter.value().nextHop[i]);
			
			}
			entry++;		
		}
		*/
		_timerPortUpdate.schedule_after_sec(_periodPort);
		_timerUpdate.schedule_after_sec(_periodUpdate);
	}else if(timer == &_timerPortHello){ //send each port
		_port_cnt_hello++;
		if(_port_cnt_hello < _num_port){
			WritablePacket *hello = Packet::make(0,0,sizeof(struct PacketHeader), 0);
			memset(hello->data(),0,hello->length());
			struct PacketHeader *header = (struct PacketHeader*) hello->data();
			header->type = 1;
			header->source = _my_address;
			header->sequence = _seq;
			header->port = _port_cnt_hello;
			output(0).push(hello);
			_timerPortHello.schedule_after_sec(_periodPort);
		}
		
	}else if(timer == &_timerPortUpdate){
		_port_cnt_update++;
		if(_port_cnt_update < _num_port){
			WritablePacket *update = Packet::make(0,0,sizeof(struct PacketHeader), 0);
			memset(update->data(),0,update->length());
			struct PacketHeader *header = (struct PacketHeader*) update->data();
			header->type = 2; //update packet
			header->source = _my_address;
			header->sequence = _seq;
			header->port = _port_cnt_update;
			header->length = sizeof(_routing_table)*_routing_table.size();
			HashTable<uint16_t, struct TableEntry> *data = (HashTable<uint16_t, TableEntry> *)(update->data() + sizeof(struct PacketHeader));
			memcpy(data, &_routing_table, sizeof(_routing_table)*_routing_table.size());
			output(1).push(update);
			_timerPortUpdate.schedule_after_sec(_periodPort);
		}
	}
}

void BasicRouter::push(int port, Packet *packet) { //receiving a hello packet
	assert(packet);
	if(port == 0){ //hello message
		struct PacketHeader *header = (struct PacketHeader *)packet->data();
		/*Receiving hello packet*/
		click_chatter("Received Hello from %u with seq %u\n", header->source, header->sequence);
		if(_routing_table.find(header->source) == _routing_table.end()){
			struct TableEntry _topo_entry_tmp;
			_topo_entry_tmp.cost = 1;
			_topo_entry_tmp.nextHop.push_back(header->port);
			_routing_table.set(header->source, _topo_entry_tmp);
		}
		/*
		click_chatter("Router %u routing table", _my_address);
		int entry = 0;
		for(HashTable<uint16_t, struct TableEntry>::iterator _topo_table_iter = _routing_table.begin(); _topo_table_iter != _routing_table.end(); ++_topo_table_iter){
			for(int i = 0; i < _topo_table_iter.value().nextHop.size(); i++){
				click_chatter("*********************Entry %d: Dst %u Cost %d NextHop %d*********************", entry, _topo_table_iter.key(), _topo_table_iter.value().cost, _topo_table_iter.value().nextHop[i]);
			
			}
			entry++;		
		}
		*/
		/*Send back ack packet*/
		click_chatter("Send ACK packet back");
		WritablePacket *ack = Packet::make(0,0,sizeof(struct PacketHeader), 0);
		memset(ack->data(),0,ack->length());
		struct PacketHeader *format = (struct PacketHeader*) ack->data();
		format->type = 3; //ACK packet
		format->sequence = header->sequence;
		format->source = _my_address;
		format->destination1 = header->source;
		format->port = header->port;
		packet->kill();
		output(0).push(ack);	

	}else if(port == 1){//update message
		struct PacketHeader *header = (struct PacketHeader *)packet->data();
		/*Receiving update packet*/
		click_chatter("Received Update from %u with seq %u", header->source, header->sequence);
		HashTable<uint16_t, struct TableEntry> *_update_table_ptr = (HashTable<uint16_t, struct TableEntry> *)(packet->data() + sizeof(struct PacketHeader));
		HashTable<uint16_t, struct TableEntry> _update_table;
		memcpy(&_update_table, _update_table_ptr, header->length);

		bool flag;
	 
		/*Adds 1 to all costs in the update table*/
		for(HashTable<uint16_t, struct TableEntry>::iterator _update_addr_iter = _update_table.begin(); _update_addr_iter != _update_table.end(); ++_update_addr_iter){
			struct TableEntry _update_entry = _update_table.get(_update_addr_iter.key());
			_update_entry.cost++;
			_update_table.set(_update_addr_iter.key(), _update_entry);
		}

		/*Compare update table with local routing table*/
		for(HashTable<uint16_t, struct TableEntry>::iterator _update_addr_iter = _update_table.begin(); _update_addr_iter != _update_table.end(); ++_update_addr_iter){
			/*Address in the update entry is not present in local routing table*/
			if(_routing_table.find(_update_addr_iter.key()) == _routing_table.end()){
				_routing_table.set(_update_addr_iter.key(), _update_addr_iter.value());
			}else{ /*Address in the update entry is present in local routing table*/
				struct TableEntry _update_entry = _update_table.get(_update_addr_iter.key());
				struct TableEntry _routing_entry = _routing_table.get(_update_addr_iter.key());
				if(_update_entry.cost == _routing_entry.cost){ //add entry of update table with different next hop to local routing table
					for(int i = 0; i < _update_entry.nextHop.size(); i++){
						flag = true;
						for(int j = 0; j < _routing_entry.nextHop.size(); j++){
							if(_update_entry.nextHop[i] == _routing_entry.nextHop[j]){
								flag = false;
								break;
							}
						}
						if(flag){
							_routing_entry.nextHop.push_back(_update_entry.nextHop[i]);
						}
					}
				}else if(_update_entry.cost < _routing_entry.cost){ //replace entry of local routing table with entry of update table
					struct TableEntry _new_routing_entry;
					_new_routing_entry.cost = _update_entry.cost;
					for(int i = 0; i < _update_entry.nextHop.size(); i++){
						_new_routing_entry.nextHop.push_back(_update_entry.nextHop[i]);
					}
					_routing_table.set(_update_addr_iter.key(), _new_routing_entry);
				}
			}		
		}

		//Send back ack packet
		click_chatter("Send ACK packet back");
		WritablePacket *ack = Packet::make(0,0,sizeof(struct PacketHeader), 0);
		memset(ack->data(),0,ack->length());
		struct PacketHeader *format = (struct PacketHeader*) ack->data();
		format->type = 3; //ACK packet
		format->sequence = header->sequence;
		format->source = _my_address;
		format->destination1 = header->source;
		format->port = header->port;
		packet->kill();
		output(1).push(ack);	
	}else if(port == 2){ //data message


	}
	
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicRouter)
