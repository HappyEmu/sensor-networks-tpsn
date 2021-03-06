#ifndef TPSN_H_
#define TPSN_H_

#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "tpsn.h"

typedef enum {
    DISCOVERY = 0, SYNC_PULSE = 1, SYNC_REQ = 2, SYNC_ACK = 3
} MessageType;

typedef struct {
    MessageType type;
    uint16_t level;
    uint16_t broadcast_id;
    uint16_t sender_id;
} DiscoveryMessage;

typedef struct {
    MessageType type;
    uint16_t sender_id;
} SyncPulseMessage;

typedef struct {
    MessageType type;
    uint16_t sender_id, destination_id;
    clock_time_t t1;
} SyncRequestMessage;

typedef struct {
    MessageType type;
    uint16_t sender_id, destination_id;
    clock_time_t t1, t2, t3;
} SyncAckMessage;

typedef struct {
    MessageType type;
    uint16_t data[200];
} AbstractMessage;

static void handle_discovery(DiscoveryMessage);
static void handle_sync_pulse();
static void handle_sync_ack(SyncAckMessage);
static void handle_sync_req(SyncRequestMessage);
static void sync();

#endif
