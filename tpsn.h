#ifndef TPSN_H_
#define TPSN_H_

rimeaddr_t getDestAddr(unsigned short);
void send_discovery_message();
void initiate_discovery();

typedef enum {
	DISCOVERY = 0, SYNC_PULSE = 1, SYNC_REQ = 2, SYNC_ACK = 3
} MessageType;

typedef struct {
	MessageType type = DISCOVERY;
	uint8_t level;
	uint16_t broadcast_id;
	uint8_t sender_id;
} DiscoveryMessage;

typedef struct {
	MessageType type = SYNC_PULSE;
	uint8_t sender_id;
} SyncPulseMessage;

typedef struct {
	MessageType type = SYNC_REQ;
	uint8_t sender_id;
	clock_time_t t1;
} SyncRequestMessage;

typedef struct {
	MessageType type = SYNC_ACK;
	uint8_t sender_id;
	clock_time_t t1, t2, t3;
} SyncRequestMessage;

typedef struct {
	clock_time_t time;
	unsigned short originator;
} TimeMessage;


#endif
