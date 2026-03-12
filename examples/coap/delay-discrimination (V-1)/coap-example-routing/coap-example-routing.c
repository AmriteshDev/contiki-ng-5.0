/*
 * Erbium (Er) CoAP Example with Routing Information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "net/app-layer/coap/coap-engine.h"

#include "res-routing-gateway.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CoAPApp"
#define LOG_LEVEL LOG_LEVEL_APP

/*
 * Welcome resource
 */
static void
welcome_handler(coap_message_t *request, coap_message_t *response,
                uint8_t *buffer, uint16_t preferred_size,
                int32_t *offset)
{
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  const char *welcome_msg = 
    "CoAP Server with Routing Info\n"
    "Available:\n"
    "  /rpl/routes - All routes\n"
    "  /rpl/gateway - Gateway\n"
    "  /rpl/neighbors - Neighbors\n"
    "  /rpl/routes/onehop - 1-hop\n"
    "  /rpl/routes/multihop - Multi-hop\n";
  
  coap_set_payload(response, (uint8_t *)welcome_msg, strlen(welcome_msg));
}

RESOURCE(res_welcome,
         "title=\"CoAP Routing\";rt=\"text\"",
         welcome_handler,
         NULL,
         NULL,
         NULL);

PROCESS(er_example_server, "Erbium Example Server");
AUTOSTART_PROCESSES(&er_example_server);

PROCESS_THREAD(er_example_server, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  LOG_INFO("Starting Erbium Example Server\n");

  coap_engine_init();
  coap_activate_resource(&res_welcome, "");
  
  /* Initialize routing resources */
  gateway_routing_resources_init();

  LOG_INFO("Server ready\n");

  PROCESS_END();
}