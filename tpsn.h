#ifndef TPSN_H_
#define TPSN_H_

rimeaddr_t getDestAddr(unsigned short);
void send_discovery_message();
void initiate_discovery();

typedef struct {
	uint8_t level;
	uint16_t broadcast_id;
	uint8_t sender_id;
} DiscoveryMessage;

typedef struct {
	clock_time_t time;
	unsigned short originator;
} TimeMessage;


#endif
