// #include "contiki.h"
// #include "coap-engine.h"
// #include "coap-blocking-api.h"
// #include "net/routing/routing.h"
// #include "sys/etimer.h"
// #include "os/sys/log.h"

// #include <string.h>
// #include <stdbool.h>

// #define LOG_MODULE "Gateway"
// #define LOG_LEVEL  LOG_LEVEL_INFO


// /* ---------------- Configuration ---------------- */

// #define K_VAL 4                 /* anomaly multiplier */
// #define MAX_NODES 2             /* normal + malicious */


// /* Explicitly defined nodes (from your topology) */
// typedef struct {
//   const char *url;
//   uint32_t srtt;
//   uint32_t rttvar;
//   bool initialized;
// } node_stat_t;


// /* Node table */
// static node_stat_t node_table[MAX_NODES] = {

//   {"coap://[fd00::c30c:0:0:2]", 0, 0, false},   /* Normal node */
//   {"coap://[fd00::c30c:0:0:7]", 0, 0, false}    /* Malicious node */

// };


// /* Current node being monitored */
// static node_stat_t *current_target = NULL;


// /* Time when ping was sent */
// static clock_time_t t_send;



// /* ---------------- CoAP Response Handler ---------------- */

// void
// client_chunk_handler(coap_message_t *response)
// {
//   clock_time_t t_reply;
//   uint32_t rtt_curr;
//   uint32_t delta;
//   uint32_t threshold;

//   if(response == NULL || current_target == NULL) {

//     LOG_ERR("Request timeout or failed.\n");
//     return;
//   }


//   /* Compute RTT */

//   t_reply = clock_time();

//   rtt_curr =
//     (uint32_t)(((t_reply - t_send) * 1000UL) / CLOCK_SECOND);


//   LOG_INFO("RTT measured: %lu ms\n",
//            (unsigned long)rtt_curr);


//   /* -------- Baseline Initialization -------- */

//   if(!current_target->initialized) {

//   /* Ignore only the very first routing setup delay */
//   if(current_target->srtt == 0 && rtt_curr > 2000) {
//     LOG_INFO("Ignoring routing startup delay\n");
//     return;
//   }

//   current_target->srtt = rtt_curr;
//   current_target->rttvar = rtt_curr / 2;
//   current_target->initialized = true;

//   LOG_INFO("Baseline initialized: %lu ms\n",
//            (unsigned long)rtt_curr);

//   return;
// }
//   /* -------- Update RTT Statistics -------- */

//   delta =
//     (current_target->srtt > rtt_curr) ?
//     (current_target->srtt - rtt_curr) :
//     (rtt_curr - current_target->srtt);


//   /* RTTVAR update */

//   current_target->rttvar =
//       ((current_target->rttvar * 3) / 4) +
//       (delta / 4);


//   /* SRTT update */

//   current_target->srtt =
//       ((current_target->srtt * 7) / 8) +
//       (rtt_curr / 8);


//   /* Compute adaptive threshold */

//   threshold =
//       current_target->srtt +
//       (K_VAL * current_target->rttvar);


//   LOG_INFO("Stats -> SRTT=%lu  RTTVAR=%lu  TH=%lu\n",
//            (unsigned long)current_target->srtt,
//            (unsigned long)current_target->rttvar,
//            (unsigned long)threshold);


//   /* -------- Delay Anomaly Detection -------- */

//   if(rtt_curr > threshold) {

//     LOG_WARN("🚨 DELAY ANOMALY detected at %s\n",
//              current_target->url);

//   } else {

//     LOG_INFO("Network normal\n");
//   }
// }



// /* ---------------- Gateway Process ---------------- */

// PROCESS(gateway_process, "Gateway Node");
// AUTOSTART_PROCESSES(&gateway_process);


// PROCESS_THREAD(gateway_process, ev, data)
// {
//   static struct etimer ping_timer;

//   static coap_endpoint_t server_ep;
//   static coap_message_t  request[1];

//   static int node_index = 0;


//   PROCESS_BEGIN();


//   LOG_INFO("Gateway starting...\n");


//   /* Start RPL root */

//   NETSTACK_ROUTING.root_start();


//   /* Wait for network formation */

//   etimer_set(&ping_timer, 15 * CLOCK_SECOND);


//   while(1) {

//     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ping_timer));


//     /* Select node */

//     current_target = &node_table[node_index];


//     LOG_INFO("Sending ping to %s\n",
//              current_target->url);


//     /* Parse endpoint */

//     coap_endpoint_parse(
//         current_target->url,
//         strlen(current_target->url),
//         &server_ep
//     );


//     /* Prepare CoAP request */

//     coap_init_message(
//         request,
//         COAP_TYPE_CON,
//         COAP_GET,
//         0
//     );


//     /* Resource name (NO slash) */

//     coap_set_header_uri_path(request, "ping");


//     /* Record send time */

//     t_send = clock_time();


//     /* Send blocking request */

//     COAP_BLOCKING_REQUEST(
//         &server_ep,
//         request,
//         client_chunk_handler
//     );


//     /* Move to next node */

//     node_index =
//         (node_index + 1) % MAX_NODES;


//     /* Next ping after 5 seconds */

//     etimer_set(&ping_timer, 5 * CLOCK_SECOND);
//   }


//   PROCESS_END();
// }





// #include "contiki.h"
// #include "coap-engine.h"
// #include "coap-blocking-api.h"
// #include "net/routing/routing.h"
// #include "net/ipv6/uip-ds6.h"
// #include "net/ipv6/uip-ds6-nbr.h"
// #include "net/ipv6/uip-ds6-route.h"
// #include "sys/etimer.h"
// #include "os/sys/log.h"
// #include <string.h>
// #include "net/routing/rpl-lite/rpl-neighbor.h"

// #include "net/nbr-table.h"

// #define LOG_MODULE "Gateway"
// #define LOG_LEVEL LOG_LEVEL_INFO

// /* Configuration */
// #define MAX_NODES 16
// #define PING_INTERVAL 10
// #define DISCOVERY_INTERVAL 15
// #define STARTUP_DISCOVERY 5
// #define TIMEOUT_LIMIT 3
// #define K_VAL 4

// typedef struct {
//   uip_ipaddr_t addr;
//   uint8_t active;
//   uint8_t timeout;
// } node_entry_t;

// typedef struct {
//   uint16_t srtt;
//   uint16_t rttvar;
//   uint8_t initialized;
// } rtt_state_t;

// static node_entry_t node_table[MAX_NODES];
// static rtt_state_t rtt_table[MAX_NODES];

// static coap_endpoint_t ep;
// static coap_message_t req;

// static clock_time_t send_time;
// static uint8_t current_node;
// static uint8_t next_node = 1;

// #define U16_DELTA(a,b) ((a)>(b)?(a)-(b):(b)-(a))

// /* Extract node ID from IPv6 */
// static uint8_t get_node_id(const uip_ipaddr_t *addr)
// {
//   return addr->u8[15];
// }

// /* Register node */
// static void register_node(const uip_ipaddr_t *addr)
// {
//   uint8_t id = get_node_id(addr);

//   if(id == 0 || id >= MAX_NODES)
//     return;

//   if(uip_ds6_is_my_addr((uip_ipaddr_t *)addr))
//     return;

//   if(!node_table[id].active) {
//     LOG_INFO("New node discovered: %u\n", id);
//   }

//   uip_ipaddr_copy(&node_table[id].addr, addr);
//   node_table[id].active = 1;
// }

// // /* Print neighbor table */
// // static void print_neighbors(void)
// // {
// //   uip_ds6_nbr_t *nbr;
// //   uip_ipaddr_t global;

// //   LOG_INFO("Neighbor table:\n");

// //   for(nbr = uip_ds6_nbr_head(); nbr; nbr = uip_ds6_nbr_next(nbr)) {

// //     LOG_INFO("Neighbor: ");
// //     LOG_INFO_6ADDR(&nbr->ipaddr);
// //     LOG_INFO_("\n");

// //     /* Convert link-local to global address */
// //     uip_ipaddr_copy(&global, &nbr->ipaddr);

// //     global.u16[0] = UIP_HTONS(0xfd00);
// //     global.u16[1] = 0;
// //     global.u16[2] = 0;
// //     global.u16[3] = 0;

// //     register_node(&global);
// //   }
// // }

// /* Print routing table */
// // static void print_routes(void)
// // {
// //   uip_ds6_route_t *route;

// //   LOG_INFO("Routing table:\n");

// //   for(route = uip_ds6_route_head(); route;
// //       route = uip_ds6_route_next(route)) {

// //     LOG_INFO("Route to: ");
// //     LOG_INFO_6ADDR(&route->ipaddr);
// //     LOG_INFO_("\n");

// //      // ADD THIS:
// //     LOG_INFO("Route prefix len: %u, last byte: %u\n",
// //              route->length, route->ipaddr.u8[15]); 

// //     register_node(&route->ipaddr);
// //   }
// // }

// /* Discover nodes */

// /* Discover nodes - scan both neighbors AND RPL neighbor set */
// static void discover_nodes(void)
// {
//   uip_ds6_nbr_t *nbr;
//   rpl_nbr_t *rpl_nbr;
//   uip_ds6_route_t *route;
//   uip_ipaddr_t global;
//   uint8_t i, count = 0;

//   LOG_INFO("\n===== DISCOVERY START =====\n");

//   /* Scan direct IPv6 neighbors */
//   LOG_INFO("Neighbor table:\n");
//   for(nbr = uip_ds6_nbr_head(); nbr; nbr = uip_ds6_nbr_next(nbr)) {
//     LOG_INFO("Neighbor: ");
//     LOG_INFO_6ADDR(&nbr->ipaddr);
//     LOG_INFO_("\n");
//     uip_ipaddr_copy(&global, &nbr->ipaddr);
//     global.u16[0] = UIP_HTONS(0xfd00);
//     global.u16[1] = 0;
//     global.u16[2] = 0;
//     global.u16[3] = 0;
//     register_node(&global);
//   }

//   /* Scan RPL neighbor table (direct RPL neighbors) */
//   LOG_INFO("RPL neighbor table:\n");
//   for(rpl_nbr = nbr_table_head(rpl_neighbors);
//       rpl_nbr != NULL;
//       rpl_nbr = nbr_table_next(rpl_neighbors, rpl_nbr)) {
//     uip_ipaddr_t *addr = rpl_neighbor_get_ipaddr(rpl_nbr);
//     if(addr != NULL) {
//       LOG_INFO("RPL nbr: ");
//       LOG_INFO_6ADDR(addr);
//       LOG_INFO_("\n");
//       if(uip_is_addr_linklocal(addr)) {
//         uip_ipaddr_copy(&global, addr);
//         global.u16[0] = UIP_HTONS(0xfd00);
//         global.u16[1] = 0;
//         global.u16[2] = 0;
//         global.u16[3] = 0;
//         register_node(&global);
//       } else {
//         register_node(addr);
//       }
//     }
//   }

//   /* Scan routing table for multi-hop nodes (nodes 2, 6 etc.) */
//     /* Scan routing table for multi-hop nodes (nodes 2, 6 etc.) */
//   for(route = uip_ds6_route_head(); route != NULL;
//       route = uip_ds6_route_next(route)) {
//     if(!uip_is_addr_linklocal(&route->ipaddr)) {
//       register_node(&route->ipaddr);
//     }
//   }

//   for(i = 1; i < MAX_NODES; i++) {
//     if(node_table[i].active) count++;
//   }

//   LOG_INFO("Discovery complete: %u nodes active\n", count);
//   LOG_INFO("===========================\n");
// }



// /* Response handler */
// void client_chunk_handler(coap_message_t *response)
// {
//   node_entry_t *n=&node_table[current_node];
//   rtt_state_t *s=&rtt_table[current_node];

//   uint16_t rtt,delta,th;

//   if(!response) {

//     n->timeout++;

//     LOG_WARN("Timeout from node %u (%u)\n",
//              current_node,n->timeout);

//     if(n->timeout>=TIMEOUT_LIMIT) {

//       LOG_WARN("Node %u considered DEAD\n",
//                current_node);

//       n->active=0;
//     }

//     return;
//   }

//   rtt=((clock_time()-send_time)*1000)/CLOCK_SECOND;

//   LOG_INFO("Response from node %u\n",current_node);
//   LOG_INFO("Measured RTT = %u ms\n",rtt);

//   n->timeout=0;

//   if(!s->initialized) {

//     s->srtt=rtt;
//     s->rttvar=rtt/2;
//     s->initialized=1;

//     LOG_INFO("Baseline RTT for node %u = %u\n",
//              current_node,rtt);

//     return;
//   }

// delta = U16_DELTA(s->srtt,rtt);

//   s->rttvar=(3*s->rttvar+delta)/4;
//   s->srtt=(7*s->srtt+rtt)/8;

//   th=s->srtt+(K_VAL*s->rttvar);

//   LOG_INFO("Updated SRTT = %u\n",s->srtt);
//   LOG_INFO("Updated RTTVAR = %u\n",s->rttvar);
//   LOG_INFO("Threshold = %u\n",th);

//   if(rtt>th) {

//     LOG_WARN("DELAY ANOMALY DETECTED for node %u\n",
//              current_node);
//   }
// }

// /* Gateway process */

// PROCESS(gateway_process,"Gateway");
// AUTOSTART_PROCESSES(&gateway_process);

// PROCESS_THREAD(gateway_process,ev,data)
// {
//   static struct etimer ping_timer;
//   static struct etimer disc_timer;

//   uint8_t id,start;

//   PROCESS_BEGIN();

//   LOG_INFO("Gateway starting\n");

//   NETSTACK_ROUTING.root_start();

//   etimer_set(&disc_timer,STARTUP_DISCOVERY*CLOCK_SECOND);
//   etimer_set(&ping_timer,PING_INTERVAL*CLOCK_SECOND);

//   while(1) {

//     PROCESS_WAIT_EVENT_UNTIL(
//       etimer_expired(&disc_timer) ||
//       etimer_expired(&ping_timer));

//     if(etimer_expired(&disc_timer)) {

//       discover_nodes();

//       etimer_set(&disc_timer,
//                  DISCOVERY_INTERVAL*CLOCK_SECOND);
//     }

//     if(etimer_expired(&ping_timer)) {

//       LOG_INFO("Selecting node to ping\n");

//       id=0;
//       start=next_node;

//       do{

//         if(node_table[next_node].active){
//           id=next_node;
//           break;
//         }

//         if(++next_node>=MAX_NODES)
//           next_node=1;

//       }while(next_node!=start);

//       if(id){

//         current_node=id;

//         memset(&ep,0,sizeof(ep));
//         memset(&req,0,sizeof(req));

//         uip_ipaddr_copy(&ep.ipaddr,
//                         &node_table[id].addr);

//         ep.port=UIP_HTONS(COAP_DEFAULT_PORT);

//         LOG_INFO("Sending ping to node %u\n",id);

//         LOG_INFO("Destination: ");
//         LOG_INFO_6ADDR(&node_table[id].addr);
//         LOG_INFO_("\n");

//         coap_init_message(&req,
//                           COAP_TYPE_CON,
//                           COAP_GET,
//                           0);

//         coap_set_header_uri_path(&req,"ping");

//         send_time=clock_time();

//         COAP_BLOCKING_REQUEST(&ep,&req,
//                               client_chunk_handler);
//       }

//       if(++next_node>=MAX_NODES)
//         next_node=1;

//       etimer_reset(&ping_timer);
//     }
//   }

//   PROCESS_END();
// }
























/*
 * RPL Border Router with CoAP Gateway Server
 * Exposes routing information through CoAP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "rpl-border-router.h"

/* Import routing resource */
#include "res-routing-gateway.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "GatewayApp"
#define LOG_LEVEL LOG_LEVEL_INFO

/* ==================== WELCOME RESOURCE ==================== */
static void
welcome_handler(coap_message_t *request, coap_message_t *response,
                uint8_t *buffer, uint16_t preferred_size,
                int32_t *offset)
{
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  const char *welcome_msg = 
    "=== CoAP Gateway Server ===\n"
    "Available Resources:\n"
    "/network/gateway - Gateway info\n"
    "/network/routes - All routes\n"
    "/network/neighbors - One-hop neighbors\n"
    "/network/routes/onehop - One-hop routes\n"
    "/network/routes/multihop - Multi-hop routes\n"
    "/.well-known/core - CoAP Service Discovery\n";
  
  coap_set_payload(response, (uint8_t *)welcome_msg, strlen(welcome_msg));
}

RESOURCE(res_welcome,
         "title=\"CoAP Gateway\";rt=\"gateway\"",
         welcome_handler,
         NULL,
         NULL,
         NULL);

/* ==================== MAIN PROCESS ==================== */

PROCESS(gateway_coap_process, "CoAP Gateway Server");
AUTOSTART_PROCESSES(&gateway_coap_process);

PROCESS_THREAD(gateway_coap_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("Starting CoAP Gateway Server\n");

  /* Initialize CoAP engine */
  coap_engine_init();

  /* Activate welcome resource */
  coap_activate_resource(&res_welcome, "");

  /* Initialize routing resources */
  gateway_routing_resources_init();

  /* Start RPL border router */
  LOG_INFO("Starting RPL Border Router\n");
  rpl_border_router_init();

  LOG_INFO("=== CoAP Gateway Ready ===\n");
  LOG_INFO("Access gateway at: coap://[<gateway-ipv6-address>]\n");

  PROCESS_END();
}