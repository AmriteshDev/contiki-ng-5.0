#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip-ds6-route.h" 
#include "sys/etimer.h"
#include "os/sys/log.h"
#include <string.h>

#define LOG_MODULE "Gateway"
#define LOG_LEVEL LOG_LEVEL_INFO

#define K_VAL 4
#define MAX_NODES 3

typedef struct {
  uip_ipaddr_t ipaddr;
  uint32_t srtt;
  uint32_t rttvar;
  bool initialized;
  bool in_use;
} node_stat_t;

static node_stat_t node_table[MAX_NODES];
static node_stat_t *current_target = NULL;
static clock_time_t t_send;

static node_stat_t* get_or_add_node(const uip_ipaddr_t *ipaddr) {
  int empty_idx = -1;
  for(int i = 0; i < MAX_NODES; i++) {
    if(node_table[i].in_use && uip_ip6addr_cmp(&node_table[i].ipaddr, ipaddr)) {
      return &node_table[i];
    }
    if(!node_table[i].in_use && empty_idx == -1) {
      empty_idx = i;
    }
  }
  if(empty_idx != -1) {
    uip_ipaddr_copy(&node_table[empty_idx].ipaddr, ipaddr);
    node_table[empty_idx].in_use = true;
    node_table[empty_idx].initialized = false;
    LOG_INFO("New node added to tracking table at index %d.\n", empty_idx);
    return &node_table[empty_idx];
  }
  return NULL; 
}

void client_chunk_handler(coap_message_t *response)
{
  if(response == NULL || current_target == NULL) {
    LOG_ERR("Request to current target timed out or failed.\n");
    return;
  }

  clock_time_t t_reply = clock_time();
  uint32_t rtt_curr = (uint32_t)(((t_reply - t_send) * 1000) / CLOCK_SECOND); 
  
  LOG_INFO("Pong received! RTT_curr: %lu ms\n", (unsigned long)rtt_curr);

  if (!current_target->initialized) {
    current_target->srtt = rtt_curr;
    current_target->rttvar = rtt_curr / 2;
    current_target->initialized = true;
  } else {
    uint32_t delta = (current_target->srtt > rtt_curr) ? 
                     (current_target->srtt - rtt_curr) : 
                     (rtt_curr - current_target->srtt);
                     
    current_target->rttvar = ((current_target->rttvar * 3) / 4) + (delta / 4);
    current_target->srtt = ((current_target->srtt * 7) / 8) + (rtt_curr / 8);
  }

  uint32_t threshold = current_target->srtt + (K_VAL * current_target->rttvar); 

  LOG_INFO("Stats -> SRTT: %lu ms | RTTVAR: %lu ms | Threshold: %lu ms\n", 
           (unsigned long)current_target->srtt, (unsigned long)current_target->rttvar, (unsigned long)threshold);

  if (rtt_curr > threshold) {
    LOG_WARN("ANOMALY DETECTED: RTT (%lu ms) exceeds threshold (%lu ms)!\n", 
             (unsigned long)rtt_curr, (unsigned long)threshold);
  } else {
    LOG_INFO("Network normal for this link.\n");
  }
}

PROCESS(gateway_process, "Gateway Node Process");
AUTOSTART_PROCESSES(&gateway_process);

PROCESS_THREAD(gateway_process, ev, data)
{
  static struct etimer ping_timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int current_node_idx = 0;

  PROCESS_BEGIN();

  LOG_INFO("Starting Dynamic Gateway Node (Ultra-Optimized Z1)...\n");

  NETSTACK_ROUTING.root_start();
  memset(node_table, 0, sizeof(node_table));

  etimer_set(&ping_timer, 15 * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&ping_timer)) {
      
      uip_ds6_route_t *r;
      for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
        get_or_add_node(&r->ipaddr);
      }

      int start_idx = current_node_idx;
      bool target_found = false;
      do {
        if(node_table[current_node_idx].in_use) {
          target_found = true;
          break;
        }
        current_node_idx = (current_node_idx + 1) % MAX_NODES;
      } while(current_node_idx != start_idx);

      if(target_found) {
        current_target = &node_table[current_node_idx];
        
        /* Direct memory assignment saves massive stack space compared to snprintf */
        memset(&server_ep, 0, sizeof(server_ep));
        uip_ipaddr_copy(&server_ep.ipaddr, &current_target->ipaddr);
        server_ep.port = UIP_HTONS(COAP_DEFAULT_PORT);
        server_ep.secure = 0;
        
        LOG_INFO("Sending Ping to node tracking index %d...\n", current_node_idx);

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/ping");

        t_send = clock_time(); 
        COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

        current_node_idx = (current_node_idx + 1) % MAX_NODES;
      }

      etimer_set(&ping_timer, 5 * CLOCK_SECOND);
    }
  }

  PROCESS_END();
}