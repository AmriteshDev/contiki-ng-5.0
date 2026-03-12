/*
 * Header file for routing gateway resource
 */

#ifndef RES_ROUTING_GATEWAY_H_
#define RES_ROUTING_GATEWAY_H_

#include "net/app-layer/coap/coap-engine.h"

extern coap_resource_t res_routing_table;
extern coap_resource_t res_gateway_info;
extern coap_resource_t res_onehop;
extern coap_resource_t res_multihop;
extern coap_resource_t res_neighbors;

void gateway_routing_resources_init(void);

#endif /* RES_ROUTING_GATEWAY_H_ */