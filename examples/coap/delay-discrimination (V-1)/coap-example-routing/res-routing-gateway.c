/*
 * Routing table exposure resource for CoAP Gateway
 * Exposes routing information through CoAP endpoints
 */

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "net/app-layer/coap/coap-engine.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-ds6.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RoutingGW"
#define LOG_LEVEL LOG_LEVEL_INFO

/* ==================== ROUTING TABLE HANDLER ==================== */
static void
routing_table_handler(coap_message_t *request, coap_message_t *response,
                      uint8_t *buffer, uint16_t preferred_size,
                      int32_t *offset)
{
  int len = 0;
  uip_ds6_route_t *route;
  int i;
  
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  len += snprintf((char *)buffer + len, preferred_size - len,
                  "=== ROUTING TABLE ===\n\n");
  
  int count = 0;
  for(route = uip_ds6_route_head(); route != NULL; 
      route = uip_ds6_route_next(route)) {
    count++;
    
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Route %d:\n", count);
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "  Dest: ");
    
    for(i = 0; i < 8; i++) {
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "%04x%s", 
                      route->ipaddr.u16[i],
                      i < 7 ? ":" : "\n");
    }
    
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "  Hops: %d\n", route->length);
    
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "  Type: %s\n\n", 
                    route->length == 1 ? "ONE-HOP" : "MULTI-HOP");
    
    if(len > preferred_size - 80) {
      break;
    }
  }
  
  if(count == 0) {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "No routes available\n");
  } else {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Total routes: %d\n", count);
  }
  
  coap_set_payload(response, buffer, len);
}

/* ==================== GATEWAY INFO HANDLER ==================== */
static void
gateway_handler(coap_message_t *request, coap_message_t *response,
                uint8_t *buffer, uint16_t preferred_size,
                int32_t *offset)
{
  int len = 0;
  int i;
  int j;
  uint8_t state;
  
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  len += snprintf((char *)buffer + len, preferred_size - len,
                  "=== GATEWAY INFORMATION ===\n\n");
  
  uip_ds6_defrt_t *defrt = uip_ds6_defrt_head();
  
  if(defrt != NULL) {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Gateway IPv6: ");
    for(i = 0; i < 8; i++) {
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "%04x%s", 
                      defrt->ipaddr.u16[i],
                      i < 7 ? ":" : "\n");
    }
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Status: ACTIVE\n");
  } else {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Status: NO GATEWAY FOUND\n");
  }
  
  len += snprintf((char *)buffer + len, preferred_size - len,
                  "\nLocal IPv6 Addresses:\n");
  
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      len += snprintf((char *)buffer + len, preferred_size - len, "  ");
      for(j = 0; j < 8; j++) {
        len += snprintf((char *)buffer + len, preferred_size - len,
                        "%04x%s",
                        uip_ds6_if.addr_list[i].ipaddr.u16[j],
                        j < 7 ? ":" : "\n");
      }
    }
  }
  
  coap_set_payload(response, buffer, len);
}

/* ==================== ONE-HOP ROUTES HANDLER ==================== */
static void
onehop_routes_handler(coap_message_t *request, coap_message_t *response,
                      uint8_t *buffer, uint16_t preferred_size,
                      int32_t *offset)
{
  int len = 0;
  uip_ds6_route_t *route;
  int i;
  
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  len += snprintf((char *)buffer + len, preferred_size - len,
                  "=== ONE-HOP ROUTES ===\n\n");
  
  int count = 0;
  for(route = uip_ds6_route_head(); route != NULL; 
      route = uip_ds6_route_next(route)) {
    
    if(route->length == 1) {
      count++;
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "Route %d (DIRECT):\n", count);
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "  Dest: ");
      
      for(i = 0; i < 8; i++) {
        len += snprintf((char *)buffer + len, preferred_size - len,
                        "%04x%s",
                        route->ipaddr.u16[i],
                        i < 7 ? ":" : "\n");
      }
      len += snprintf((char *)buffer + len, preferred_size - len, "\n");
      
      if(len > preferred_size - 80) {
        break;
      }
    }
  }
  
  if(count == 0) {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "No one-hop routes available\n");
  } else {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Total one-hop routes: %d\n", count);
  }
  
  coap_set_payload(response, buffer, len);
}

/* ==================== MULTI-HOP ROUTES HANDLER ==================== */
static void
multihop_routes_handler(coap_message_t *request, coap_message_t *response,
                        uint8_t *buffer, uint16_t preferred_size,
                        int32_t *offset)
{
  int len = 0;
  uip_ds6_route_t *route;
  int i;
  
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  len += snprintf((char *)buffer + len, preferred_size - len,
                  "=== MULTI-HOP ROUTES ===\n\n");
  
  int count = 0;
  for(route = uip_ds6_route_head(); route != NULL; 
      route = uip_ds6_route_next(route)) {
    
    if(route->length > 1) {
      count++;
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "Route %d (VIA %d HOPS):\n", count, route->length);
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "  Dest: ");
      
      for(i = 0; i < 8; i++) {
        len += snprintf((char *)buffer + len, preferred_size - len,
                        "%04x%s",
                        route->ipaddr.u16[i],
                        i < 7 ? ":" : "\n");
      }
      len += snprintf((char *)buffer + len, preferred_size - len, "\n");
      
      if(len > preferred_size - 80) {
        break;
      }
    }
  }
  
  if(count == 0) {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "No multi-hop routes available\n");
  } else {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Total multi-hop routes: %d\n", count);
  }
  
  coap_set_payload(response, buffer, len);
}

/* ==================== NEIGHBORS HANDLER ==================== */
static void
neighbors_handler(coap_message_t *request, coap_message_t *response,
                  uint8_t *buffer, uint16_t preferred_size,
                  int32_t *offset)
{
  int len = 0;
  uip_ds6_nbr_t *nbr;
  int i;
  
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  len += snprintf((char *)buffer + len, preferred_size - len,
                  "=== NEIGHBORS (1-HOP REACHABLE) ===\n\n");
  
  int count = 0;
  for(nbr = uip_ds6_nbr_head(); nbr != NULL; 
      nbr = uip_ds6_nbr_next(nbr)) {
    count++;
    
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Neighbor %d:\n", count);
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "  IPv6: ");
    
    for(i = 0; i < 8; i++) {
      len += snprintf((char *)buffer + len, preferred_size - len,
                      "%04x%s",
                      nbr->ipaddr.u16[i],
                      i < 7 ? ":" : "\n");
    }
    
    const char *nbr_state;
    switch(nbr->state) {
      case NBR_INCOMPLETE: nbr_state = "INCOMPLETE"; break;
      case NBR_REACHABLE: nbr_state = "REACHABLE"; break;
      case NBR_STALE: nbr_state = "STALE"; break;
      case NBR_DELAY: nbr_state = "DELAY"; break;
      case NBR_PROBE: nbr_state = "PROBE"; break;
      default: nbr_state = "UNKNOWN"; break;
    }
    
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "  State: %s\n\n", nbr_state);
    
    if(len > preferred_size - 80) {
      break;
    }
  }
  
  if(count == 0) {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "No neighbors found\n");
  } else {
    len += snprintf((char *)buffer + len, preferred_size - len,
                    "Total neighbors: %d\n", count);
  }
  
  coap_set_payload(response, buffer, len);
}

/* ==================== DEFINE CoAP RESOURCES ==================== */

RESOURCE(res_routing_table,
         "title=\"Routing Table\";rt=\"routes\"",
         routing_table_handler,
         NULL,
         NULL,
         NULL);

RESOURCE(res_gateway_info,
         "title=\"Gateway Info\";rt=\"gateway\"",
         gateway_handler,
         NULL,
         NULL,
         NULL);

RESOURCE(res_onehop,
         "title=\"One-Hop Routes\";rt=\"onehop\"",
         onehop_routes_handler,
         NULL,
         NULL,
         NULL);

RESOURCE(res_multihop,
         "title=\"Multi-Hop Routes\";rt=\"multihop\"",
         multihop_routes_handler,
         NULL,
         NULL,
         NULL);

RESOURCE(res_neighbors,
         "title=\"Neighbors\";rt=\"neighbors\"",
         neighbors_handler,
         NULL,
         NULL,
         NULL);

/* ==================== INITIALIZATION FUNCTION ==================== */

void
gateway_routing_resources_init(void)
{
  LOG_INFO("Initializing routing resources\n");
  
  coap_activate_resource(&res_routing_table, "rpl/routes");
  coap_activate_resource(&res_gateway_info, "rpl/gateway");
  coap_activate_resource(&res_onehop, "rpl/routes/onehop");
  coap_activate_resource(&res_multihop, "rpl/routes/multihop");
  coap_activate_resource(&res_neighbors, "rpl/neighbors");
  
  LOG_INFO("Resources initialized:\n");
  LOG_INFO("  /rpl/routes - All routes\n");
  LOG_INFO("  /rpl/gateway - Gateway info\n");
  LOG_INFO("  /rpl/routes/onehop - One-hop routes\n");
  LOG_INFO("  /rpl/routes/multihop - Multi-hop routes\n");
  LOG_INFO("  /rpl/neighbors - Neighbors\n");
}