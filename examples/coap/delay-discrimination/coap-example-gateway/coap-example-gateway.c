#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "net/routing/routing.h"
#include "sys/etimer.h"
#include "os/sys/log.h"

#include <string.h>
#include <stdbool.h>

#define LOG_MODULE "Gateway"
#define LOG_LEVEL  LOG_LEVEL_INFO


/* ---------------- Configuration ---------------- */

#define K_VAL 4                 /* anomaly multiplier */
#define MAX_NODES 2             /* normal + malicious */


/* Explicitly defined nodes (from your topology) */
typedef struct {
  const char *url;
  uint32_t srtt;
  uint32_t rttvar;
  bool initialized;
} node_stat_t;


/* Node table */
static node_stat_t node_table[MAX_NODES] = {

  {"coap://[fd00::c30c:0:0:2]", 0, 0, false},   /* Normal node */
  {"coap://[fd00::c30c:0:0:7]", 0, 0, false}    /* Malicious node */

};


/* Current node being monitored */
static node_stat_t *current_target = NULL;


/* Time when ping was sent */
static clock_time_t t_send;



/* ---------------- CoAP Response Handler ---------------- */

void
client_chunk_handler(coap_message_t *response)
{
  clock_time_t t_reply;
  uint32_t rtt_curr;
  uint32_t delta;
  uint32_t threshold;

  if(response == NULL || current_target == NULL) {

    LOG_ERR("Request timeout or failed.\n");
    return;
  }


  /* Compute RTT */

  t_reply = clock_time();

  rtt_curr =
    (uint32_t)(((t_reply - t_send) * 1000UL) / CLOCK_SECOND);


  LOG_INFO("RTT measured: %lu ms\n",
           (unsigned long)rtt_curr);


  /* -------- Baseline Initialization -------- */

  if(!current_target->initialized) {

  /* Ignore only the very first routing setup delay */
  if(current_target->srtt == 0 && rtt_curr > 2000) {
    LOG_INFO("Ignoring routing startup delay\n");
    return;
  }

  current_target->srtt = rtt_curr;
  current_target->rttvar = rtt_curr / 2;
  current_target->initialized = true;

  LOG_INFO("Baseline initialized: %lu ms\n",
           (unsigned long)rtt_curr);

  return;
}
  /* -------- Update RTT Statistics -------- */

  delta =
    (current_target->srtt > rtt_curr) ?
    (current_target->srtt - rtt_curr) :
    (rtt_curr - current_target->srtt);


  /* RTTVAR update */

  current_target->rttvar =
      ((current_target->rttvar * 3) / 4) +
      (delta / 4);


  /* SRTT update */

  current_target->srtt =
      ((current_target->srtt * 7) / 8) +
      (rtt_curr / 8);


  /* Compute adaptive threshold */

  threshold =
      current_target->srtt +
      (K_VAL * current_target->rttvar);


  LOG_INFO("Stats -> SRTT=%lu  RTTVAR=%lu  TH=%lu\n",
           (unsigned long)current_target->srtt,
           (unsigned long)current_target->rttvar,
           (unsigned long)threshold);


  /* -------- Delay Anomaly Detection -------- */

  if(rtt_curr > threshold) {

    LOG_WARN("🚨 DELAY ANOMALY detected at %s\n",
             current_target->url);

  } else {

    LOG_INFO("Network normal\n");
  }
}



/* ---------------- Gateway Process ---------------- */

PROCESS(gateway_process, "Gateway Node");
AUTOSTART_PROCESSES(&gateway_process);


PROCESS_THREAD(gateway_process, ev, data)
{
  static struct etimer ping_timer;

  static coap_endpoint_t server_ep;
  static coap_message_t  request[1];

  static int node_index = 0;


  PROCESS_BEGIN();


  LOG_INFO("Gateway starting...\n");


  /* Start RPL root */

  NETSTACK_ROUTING.root_start();


  /* Wait for network formation */

  etimer_set(&ping_timer, 15 * CLOCK_SECOND);


  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ping_timer));


    /* Select node */

    current_target = &node_table[node_index];


    LOG_INFO("Sending ping to %s\n",
             current_target->url);


    /* Parse endpoint */

    coap_endpoint_parse(
        current_target->url,
        strlen(current_target->url),
        &server_ep
    );


    /* Prepare CoAP request */

    coap_init_message(
        request,
        COAP_TYPE_CON,
        COAP_GET,
        0
    );


    /* Resource name (NO slash) */

    coap_set_header_uri_path(request, "ping");


    /* Record send time */

    t_send = clock_time();


    /* Send blocking request */

    COAP_BLOCKING_REQUEST(
        &server_ep,
        request,
        client_chunk_handler
    );


    /* Move to next node */

    node_index =
        (node_index + 1) % MAX_NODES;


    /* Next ping after 5 seconds */

    etimer_set(&ping_timer, 5 * CLOCK_SECOND);
  }


  PROCESS_END();
}





// #include "contiki.h"
// #include "coap-engine.h"
// #include "coap-blocking-api.h"
// #include "net/routing/routing.h"
// #include "sys/etimer.h"
// #include "os/sys/log.h"
// #include <string.h>

// #define LOG_MODULE "Gateway"
// #define LOG_LEVEL  LOG_LEVEL_WARN

// #define K_VAL      4
// #define FIRST_NODE 2
// #define LAST_NODE  7

// /* ---------------- Global Variables ---------------- */

// /*
//  * ep and req are global so COAP_BLOCKING_REQUEST (which must stay
//  * inside PROCESS_THREAD) can access them after set_endpoint()
//  * and prepare_get() have filled them in.
//  */
// static coap_endpoint_t ep;
// static coap_message_t  req[1];
// static clock_time_t    t_send;

// /* RTT statistics — uint16_t sufficient for ms values */
// static uint16_t srtt        = 0;
// static uint16_t rttvar      = 0;
// static uint8_t  initialized = 0;
// static uint8_t  phase2_flag = 0;


// /* ---------------- CoAP Response Handler ---------------- */

// /*
//  * Called when node 2 replies to /ping (or times out).
//  * Measures RTT, updates SRTT/RTTVAR, sets phase2_flag on anomaly.
//  */
// void client_chunk_handler(coap_message_t *response)
// {
//   uint16_t rtt, delta, threshold;

//   if(!response) { LOG_ERR("Timeout\n"); return; }

//   /* Calculate RTT in ms */
//   rtt = (uint16_t)(((clock_time() - t_send) * 1000UL) / CLOCK_SECOND);

//   /* Skip abnormal first measurement (routing not stable yet) */
//   if(!initialized && rtt > 2000) {
//     LOG_WARN("Startup delay ignored\n");
//     return;
//   }

//   /* First valid measurement — set baseline */
//   if(!initialized) {
//     srtt        = rtt;
//     rttvar      = rtt >> 1;   /* rttvar starts at srtt/2 */
//     initialized = 1;
//     LOG_WARN("Baseline: %u ms\n", srtt);
//     return;
//   }

//   /* Update RTTVAR = 3/4*rttvar + 1/4*delta */
//   delta  = (srtt > rtt) ? (srtt - rtt) : (rtt - srtt);
//   rttvar = (uint16_t)((3u * rttvar + delta) >> 2);

//   /* Update SRTT = 7/8*srtt + 1/8*rtt */
//   srtt   = (uint16_t)((7u * srtt + rtt) >> 3);

//   /* Anomaly threshold = SRTT + K * RTTVAR */
//   threshold = srtt + (uint16_t)(K_VAL * rttvar);

//   LOG_WARN("RTT=%u SRTT=%u RTTVAR=%u TH=%u\n", rtt, srtt, rttvar, threshold);

//   if(rtt > threshold) {
//     LOG_WARN("DELAY DETECTED\n");
//     phase2_flag = 1;
//   }
// }


// /* ---------------- Helper: set endpoint for a node ---------------- */

// /*
//  * Fills ep with fd00::c30c:0:0:<node> and default CoAP port.
//  * Uses uip_ip6addr() directly — avoids sprintf+coap_endpoint_parse
//  * which pull in heavy printf ROM on Z1.
//  */
// static void set_endpoint(uint8_t node)
// {
//   uip_ip6addr(&ep.ipaddr, 0xfd00,0,0,0,0xc30c,0,0,node);
//   ep.port = UIP_HTONS(COAP_DEFAULT_PORT);
// }


// /* ---------------- Helper: prepare CoAP GET request ---------------- */

// /*
//  * Fills req[] with a confirmable GET for the given path.
//  * COAP_BLOCKING_REQUEST must still be called in PROCESS_THREAD —
//  * this just prepares the message so the thread stays clean.
//  */
// static void prepare_get(const char *path)
// {
//   coap_init_message(req, COAP_TYPE_CON, COAP_GET, 0);
//   coap_set_header_uri_path(req, path);
// }


// /* ---------------- Gateway Process ---------------- */

// PROCESS(gw, "Gateway");
// AUTOSTART_PROCESSES(&gw);

// PROCESS_THREAD(gw, ev, data)
// {
//   static struct etimer ping_timer;
//   static uint8_t node; /* loop variable must be static in protothread */

//   PROCESS_BEGIN();

//   NETSTACK_ROUTING.root_start();
//   LOG_WARN("Gateway ready\n");

//   etimer_set(&ping_timer, 10 * CLOCK_SECOND);

//   while(1) {
//     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ping_timer));

//     /* ---- Phase-1: ping node 2 ---- */
//     set_endpoint(FIRST_NODE);
//     prepare_get("ping");
//     t_send = clock_time();
//     COAP_BLOCKING_REQUEST(&ep, req, client_chunk_handler);

//     /* ---- Phase-2: alert all nodes if anomaly detected ---- */
//     if(phase2_flag) {
//       phase2_flag = 0;
//       LOG_WARN("Phase-2 triggered\n");

//       for(node = FIRST_NODE; node <= LAST_NODE; node++) {
//         set_endpoint(node);
//         prepare_get("phase2");
//         LOG_WARN("Phase-2 -> N%u\n", node);
//         COAP_BLOCKING_REQUEST(&ep, req, NULL);
//       }
//     }

//     etimer_reset(&ping_timer);
//   }

//   PROCESS_END();
// }