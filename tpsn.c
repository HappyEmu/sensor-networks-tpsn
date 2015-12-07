#include <stdint.h>
#include "tpsn.h"

static struct ctimer leds_off_timer_send;

static void on_message_received(struct broadcast_conn *c);

static const struct broadcast_callbacks discovery_callbacks = {on_message_received};
static struct broadcast_conn bc;

static clock_time_t rtt;

static unsigned long sys_time = 0;
static struct ctimer time;

static uint16_t parent_node, level;
static uint16_t last_broadcast_id = (uint16_t) (1 << 16);

static AbstractMessage msgReceived;
static SyncPulseMessage pulse_msg;

static void timerCallback_turnOffLeds() {
    leds_off(LEDS_BLUE);
}

static void on_message_received(struct broadcast_conn *c) {
    leds_on(LEDS_BLUE);

    ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

    int i = 0;
    for (; i < 200; i++) {
        msgReceived.data[i] = (uint16_t) 0;
    }

    int copied = packetbuf_copyto(&msgReceived);
    printf("Copied from msgReceived = %d", copied);

    printf("Got new message with type %d\n", msgReceived.type);

    printf("Decoded msg is \n");
    for (i = 0; i < 20; i++) {
        printf("Byte %d is %u\n", i, msgReceived.data[i]);
    }

    switch (msgReceived.type) {
        case DISCOVERY: {
            static DiscoveryMessage disc_msg;
            packetbuf_copyto(&disc_msg);
            handle_discovery(disc_msg);

            break;
        }
        case SYNC_PULSE: {

            packetbuf_copyto(&pulse_msg);

            if (pulse_msg.sender_id != parent_node) {
                break;
            }
            static struct ctimer ct;
            clock_time_t backoff = (rand() % CLOCK_SECOND) + 1;
            ctimer_set(&ct, backoff, handle_sync_pulse, NULL);

            break;
        }
        case SYNC_REQ: {
            //TODO: CHECK DESTINATION
            static SyncRequestMessage req_msg;
            packetbuf_copyto(&req_msg);
            handle_sync_req(req_msg);

            break;
        }
        case SYNC_ACK: {
            //TODO: CHECK DESTINATION
            static SyncAckMessage ack_msg;
            packetbuf_copyto(&ack_msg);
            handle_sync_ack(ack_msg);

            break;
        }
        default:
            break;
    }
}

static void handle_discovery(DiscoveryMessage disc_message) {
    printf("Broadcast message %d received from %d\n", disc_message.broadcast_id, disc_message.sender_id);

    if (last_broadcast_id != disc_message.broadcast_id) {
        printf("Got new broadcast: %d\n", disc_message.broadcast_id);

        // Update state variables
        last_broadcast_id = disc_message.broadcast_id;
        parent_node = disc_message.sender_id;
        level = disc_message.level + (uint16_t) 1;
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

static void handle_sync_pulse() {
    if (pulse_msg.sender_id == 79) {
        printf("========== Correct sender id =================");
    }

    uint32_t id = (uint32_t) ((0x0000FFFF & pulse_msg.sender_id) >> 0);
    printf("Received sync pulse from %d\n", id);

    clock_time_t t1 = sys_time;
    printf("T1 is: %lu ticks\n", t1);

    SyncRequestMessage req_msg = {.type = SYNC_REQ, .sender_id = node_id, .destination_id = parent_node, .t1 = t1};
    packetbuf_copyfrom(&req_msg, sizeof(req_msg));
    broadcast_send(&bc);
    printf("sending sync req to %d\n", req_msg.destination_id);

}

static void handle_sync_req(SyncRequestMessage req_msg) {
    if (req_msg.destination_id != node_id) return;

    printf("Received sync request from %d\n", req_msg.sender_id);
    clock_time_t t2 = sys_time;

    SyncAckMessage sync_ack = {.type = SYNC_ACK, .sender_id = node_id, .destination_id = req_msg.sender_id, .t1 = req_msg.t1, .t2 = t2};
    clock_time_t t3 = sys_time;
    sync_ack.t3 = t3;

    printf("Times are: t1: %lu t2: %lu t3: %lu \n", req_msg.t1, t2, t3);
    packetbuf_copyfrom(&sync_ack, sizeof(sync_ack));
    broadcast_send(&bc);


    printf("sending sync ack to %d\n", sync_ack.destination_id);
}

static void handle_sync_ack(SyncAckMessage ack_msg) {
    printf("Received sync ack from %d\n", ack_msg.sender_id);
    clock_time_t t4 = sys_time;
    printf("Times are: t1: %lu t2: %lu t3: %lu t4: %lu \n", ack_msg.t1, ack_msg.t2, ack_msg.t3, t4);

    int Delta = (int) (((ack_msg.t2 - ack_msg.t1) - (t4 - ack_msg.t3)) / 2);
    printf("Delta: %d \n", Delta);

    printf("Time before: %lu \n", sys_time);

    sys_time += Delta;

    printf("Time after: %lu \n", sys_time);

}

static void reset_timer(void *ptr) {
    sys_time++;
    ctimer_reset(&time);
}

// Start processes
PROCESS(tpsn_process, "TPSN Prototype");
AUTOSTART_PROCESSES(&tpsn_process);

PROCESS_THREAD(tpsn_process, ev, data) {
    PROCESS_EXITHANDLER(broadcast_close(&bc);)

    PROCESS_BEGIN();

                broadcast_open(&bc, 146, &discovery_callbacks);

                SENSORS_ACTIVATE(button_sensor);
                ctimer_set(&time, 1, &reset_timer, NULL);

                static DiscoveryMessage msgSend;
                msgSend.sender_id = 0;
                msgSend.broadcast_id = 0;
                msgSend.level = 0;
                msgSend.type = 0;

                while (1) {
                    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

                    msgSend.broadcast_id = last_broadcast_id + (uint16_t) 1;
                    msgSend.level = 0;
                    msgSend.sender_id = node_id;

                    last_broadcast_id = msgSend.broadcast_id;

                    packetbuf_copyfrom(&msgSend, sizeof(msgSend));

                    broadcast_send(&bc);

                    printf("Sending initial discovery packet to all\n");

                    static struct etimer wait_timer;
                    etimer_set(&wait_timer, CLOCK_SECOND / 2);
                    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));

                    SyncPulseMessage pulse_msg;
                    pulse_msg.bullshit = 0;
                    pulse_msg.sender_id = node_id;
                    pulse_msg.type = SYNC_PULSE;

                    printf("Sending Sync Pulse message from %d\n", pulse_msg.sender_id);

                    packetbuf_copyfrom(&pulse_msg, sizeof(pulse_msg));
                    broadcast_send(&bc);
                }

                SENSORS_DEACTIVATE(button_sensor);

    PROCESS_END();
}
