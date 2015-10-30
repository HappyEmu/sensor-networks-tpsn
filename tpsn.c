#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include "tpsn.h"

static struct ctimer leds_off_timer_send;
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from);
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;

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

static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
	printf("unicast message received from %d.%d\n", from->u8[0], from->u8[1]);

	leds_on(LEDS_BLUE);

	ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

	packetbuf_copyto(&tmReceived);

	if (tmReceived.originator == node_id) {
		rtt = clock_time() - tmReceived.time;

		printf("RTT was %d milliseconds\n", (int)(((uint16_t) rtt / (float)CLOCK_SECOND)*1000));

	} else {
		printf("time received = %d clock ticks", (uint16_t)tmReceived.time);
		printf(" = %d secs ", (uint16_t)tmReceived.time / CLOCK_SECOND);
		printf("%d millis ", (1000L * ((uint16_t)tmReceived.time  % CLOCK_SECOND)) / CLOCK_SECOND);
		printf("originator = %d\n", tmReceived.originator);

		packetbuf_copyfrom(&tmReceived, sizeof(tmReceived));

		rimeaddr_t addr = getDestAddr(node_id);

		unicast_send(&uc, &addr);

		printf("sending ACK packet to %u\n", addr.u8[0]);
	}
}

// Start processes
PROCESS(example_unicast_process, "RTT using Rime Unicast Primitive");
AUTOSTART_PROCESSES(&example_unicast_process);

PROCESS_THREAD(example_unicast_process, ev, data)
{
	PROCESS_EXITHANDLER(unicast_close(&uc);)

	PROCESS_BEGIN();

	unicast_open(&uc, 146, &unicast_callbacks);

	SENSORS_ACTIVATE(button_sensor);

	while(1){
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

		tmSent.time = clock_time();
		tmSent.originator = node_id;

		packetbuf_copyfrom(&tmSent, sizeof(tmSent));

		rimeaddr_t addr = getDestAddr(node_id);

		unicast_send(&uc, &addr);

		printf("sending packet to %u\n", addr.u8[0]);
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
