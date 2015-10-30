#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include "tpsn.h"

static struct ctimer leds_off_timer_send;
static void on_discovery_received(struct broadcast_conn *c);
static const struct broadcast_callbacks discovery_callbacks = {on_discovery_received};
static struct broadcast_conn bc;

static clock_time_t rtt;

static uint8_t parent_node, level;
static uint16_t last_broadcast_id = 1 << 16;

static DiscoveryMessage dmReceived, dmSend;

static void timerCallback_turnOffLeds()
{
	leds_off(LEDS_BLUE);
}

static void on_discovery_received(struct broadcast_conn *c)
{
	leds_on(LEDS_BLUE);

	ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

	packetbuf_copyto(&dmReceived);
	printf("Broadcast message %d received from %d\n", dmReceived.broadcast_id, dmReceived.sender_id);

	if (last_broadcast_id != dmReceived.broadcast_id) {
		printf("Got new broadcast: %d\n", dmReceived.broadcast_id);

		// Update state variables
		last_broadcast_id = dmReceived.broadcast_id;
		parent_node = dmReceived.sender_id;
		level = dmReceived.level + 1;

		dmSend.broadcast_id = last_broadcast_id;
		dmSend.level = level;
		dmSend.sender_id = node_id;

		printf("Rebroadcasting to other nodes\n");
		packetbuf_copyfrom(&dmSend, sizeof(dmSend));

		broadcast_send(&bc);
	}
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

	while(1){
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

		dmSend.broadcast_id = last_broadcast_id + 1;
		dmSend.level = 0;
		dmSend.sender_id = node_id;

		packetbuf_copyfrom(&dmSend, sizeof(dmSend));

		broadcast_send(&bc);

		printf("Sending initial discovery packet to all");
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
