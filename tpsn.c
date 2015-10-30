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

typedef struct {
	clock_time_t time;
	unsigned short originator;
} TimeMessage;

static TimeMessage tmReceived;
static TimeMessage tmSent;

static void timerCallback_turnOffLeds()
{
	leds_off(LEDS_BLUE);
}

static void on_discovery_received(struct broadcast_conn *c)
{
	leds_on(LEDS_BLUE);

	ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

	packetbuf_copyto(&tmReceived);
	printf("unicast message received from %d\n", tmReceived.originator);

	if (tmReceived.originator == node_id) {
		rtt = clock_time() - tmReceived.time;

		printf("RTT was %d milliseconds\n", (int)(((uint16_t) rtt / (float)CLOCK_SECOND)*1000));

	} else {
		printf("time received = %d clock ticks", (uint16_t)tmReceived.time);
		printf(" = %d secs ", (uint16_t)tmReceived.time / CLOCK_SECOND);
		printf("%d millis ", (1000L * ((uint16_t)tmReceived.time  % CLOCK_SECOND)) / CLOCK_SECOND);
		printf("originator = %d\n", tmReceived.originator);

		packetbuf_copyfrom(&tmReceived, sizeof(tmReceived));

		broadcast_send(&bc);

		printf("sending ACK packet to all");
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

		tmSent.time = clock_time();
		tmSent.originator = node_id;

		packetbuf_copyfrom(&tmSent, sizeof(tmSent));

		broadcast_send(&bc);

		printf("sending packet to all");
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
