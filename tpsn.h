#ifndef TPSN_H_
#define TPSN_H_

rimeaddr_t getDestAddr(unsigned short);
static void handle_discovery(DiscoveryMessage);
static void handle_sync_pulse(SyncPulseMessage);
static void handle_sync_ack(SyncAckMessage);
static void handle_sync_req(SyncRequestMessage);
void send_discovery_message();
void initiate_discovery();

typedef enum {
	DISCOVERY = 0, SYNC_PULSE = 1, SYNC_REQ = 2, SYNC_ACK = 3
} MessageType;

typedef struct {
	MessageType type;
	uint8_t level;
	uint16_t broadcast_id;
	uint8_t sender_id;
} DiscoveryMessage;

typedef struct {
	MessageType type;
	uint8_t sender_id;
} SyncPulseMessage;

typedef struct {
	MessageType type;
	uint8_t sender_id, destination_id;
	clock_time_t t1;
} SyncRequestMessage;

typedef struct {
	MessageType type;
	uint8_t sender_id;
	clock_time_t t1, t2, t3;
} SyncAckMessage;

typedef struct {
	MessageType type;
	uint8_t data[200];
} AbstractMessage;

typedef struct {
	clock_time_t time;
	unsigned short originator;
} TimeMessage;




#endif
