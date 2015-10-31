#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include "tpsn.h"


static struct ctimer leds_off_timer_send;
static void on_message_received(struct broadcast_conn *c);
static const struct broadcast_callbacks discovery_callbacks = {on_message_received};
static struct broadcast_conn bc;

static clock_time_t rtt;

static uint8_t parent_node, level;
static uint16_t last_broadcast_id = 1 << 16;

static AbstractMessage msgReceived;

static void timerCallback_turnOffLeds()
{
	leds_off(LEDS_BLUE);
}

static void on_message_received(struct broadcast_conn *c)
{
	leds_on(LEDS_BLUE);

	ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

	packetbuf_copyto(&msgReceived);

	printf("Got new message with type %d\n", msgReceived.type);

	switch (msgReceived.type) {
		case DISCOVERY: {
			static DiscoveryMessage disc_msg;
			packetbuf_copyto(&disc_msg);
			handle_discovery(disc_msg);

			break;
		}
		case SYNC_PULSE: {
			static SyncPulseMessage pulse_msg;
			packetbuf_copyto(&pulse_msg);
			if(pulse_msg.sender_id != parent_node){
				break;
			}
			static struct ctimer ct;
			clock_time_t backoff = (rand() % CLOCK_SECOND) + 1;
			ctimer_set(&ct, backoff, &handle_sync_pulse, &pulse_msg);
			//handle_sync_pulse(pulse_msg);

			break;
		}
		case SYNC_REQ: {
			static SyncRequestMessage req_msg;
			packetbuf_copyto(&req_msg);
			handle_sync_req(req_msg);

			break;
		}
		case SYNC_ACK: {
			static SyncAckMessage ack_msg;
			packetbuf_copyto(&ack_msg);
			handle_sync_ack(ack_msg);

			break;
		}
		default: break;
	}
}

static void handle_discovery(DiscoveryMessage disc_message) {
	printf("Broadcast message %d received from %d\n", disc_message.broadcast_id, disc_message.sender_id);

	if (last_broadcast_id != disc_message.broadcast_id) {
		printf("Got new broadcast: %d\n", disc_message.broadcast_id);

		// Update state variables
		last_broadcast_id = disc_message.broadcast_id;
		parent_node = disc_message.sender_id;
		level = disc_message.level + 1;
		printf("Assigned: parent node: %d, level: %d\n", parent_node, level);

		disc_message.type = DISCOVERY;
		disc_message.broadcast_id = last_broadcast_id;
		disc_message.level = level;
		disc_message.sender_id = node_id;

		printf("Rebroadcasting to other nodes\n");
		packetbuf_copyfrom(&disc_message, sizeof(disc_message));

		broadcast_send(&bc);
	}
}

static void handle_sync_pulse(SyncPulseMessage pulse_msg) {
	printf("Received sync pulse from %d\n", pulse_msg.sender_id);

	clock_time_t t1 = clock_time();
	printf("T1 is: %u ticks\n", t1);

	SyncRequestMessage req_msg = {.type = SYNC_REQ, .sender_id = node_id, .destination_id = parent_node, .t1 = t1};
	packetbuf_copyfrom(&req_msg, sizeof(req_msg));
	broadcast_send(&bc);
	printf("sending sync req to %d\n", req_msg.destination_id);

}



static void handle_sync_req(SyncRequestMessage req_msg) {
	if(req_msg.destination_id != node_id) return;

	printf("Received sync request from %d\n", req_msg.sender_id);
	clock_time_t t2 = clock_time();

	SyncAckMessage sync_ack = {.type = SYNC_ACK, .sender_id = node_id, .destination_id = parent_node, .t1 = req_msg.t1, .t2 = t2};
	clock_time_t t3 = clock_time();
	sync_ack.t3 = t3;

	printf("Times are: t1: %u t2: %u t3: %u ", (unsigned int)req_msg.t1, (unsigned int)t2, (unsigned int)t3);
	packetbuf_copyfrom(&sync_ack, sizeof(sync_ack));
	broadcast_send(&bc);


	printf("sending sync ack to %d\n", sync_ack.destination_id);
}

static void handle_sync_ack(SyncAckMessage ack_msg) {
	printf("Received sync ack from %d\n", ack_msg.sender_id);
	clock_time_t t4 = clock_time();
	printf("Times are: t1: %u t2: %u t3: %u t4: %u \n", (unsigned int)ack_msg.t1, (unsigned int)ack_msg.t2, (unsigned int)ack_msg.t3, (unsigned int)t4);

	clock_time_t Delta = ((ack_msg.t2 - ack_msg.t1)-(t4 - ack_msg.t3))/2;
	clock_time_t d = ((ack_msg.t2 - ack_msg.t1)+(t4 - ack_msg.t3))/2;
	//clock_adjust(Delta);
	printf("local time: %u, t3: %u",(unsigned int)clock_time(),(unsigned int)ack_msg.t3);
}

// Start processes
PROCESS(tpsn_process, "TPSN Prototype");
AUTOSTART_PROCESSES(&tpsn_process);

PROCESS_THREAD(tpsn_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&bc);)

	PROCESS_BEGIN();

	broadcast_open(&bc, 146, &discovery_callbacks);

	SENSORS_ACTIVATE(button_sensor);
	clock_init();

	static DiscoveryMessage msgSend;

	while(1){
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

		msgSend.broadcast_id = last_broadcast_id + 1;
		msgSend.level = 0;
		msgSend.sender_id = node_id;

		last_broadcast_id = msgSend.broadcast_id;

		packetbuf_copyfrom(&msgSend, sizeof(msgSend));

		broadcast_send(&bc);

		printf("Sending initial discovery packet to all\n");

		static struct etimer wait_timer;
		etimer_set(&wait_timer, CLOCK_SECOND / 2);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));

		printf("Sending Sync Pulse message\n");

		SyncPulseMessage pulse_msg;
		pulse_msg.sender_id = node_id;
		pulse_msg.type = SYNC_PULSE;


		packetbuf_copyfrom(&pulse_msg, sizeof(pulse_msg));
		broadcast_send(&bc);
	}

	SENSORS_DEACTIVATE(button_sensor);

	PROCESS_END();
}

rimeaddr_t getDestAddr(unsigned short node_id) {
	rimeaddr_t addr;
	/* in case I am node 50, choose 51 as destination.*/
	if(node_id % 2 == 0) {
		addr.u8[0] = node_id + 1;
	}
	/* In case I am node 51, choose 50, etc */
	else {
		addr.u8[0] = node_id - 1;
	}
	addr.u8[1] = 0;

	return addr;
}
