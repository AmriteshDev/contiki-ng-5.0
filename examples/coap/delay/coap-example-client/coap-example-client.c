#include "contiki.h"
#include "coap-engine.h"
#include "os/sys/log.h"
#include <string.h>

#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO

/* --- Define the CoAP Resource --- */
// This creates a GET resource at the path "/ping"
RESOURCE(res_ping,
         "title=\"Ping Resource\";rt=\"Text\"",
         NULL, NULL, NULL, NULL);

// This function is triggered when the Gateway sends a GET request
static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *message = "pong";
  size_t len = strlen(message);

  // Set the payload of the response to "pong"
  coap_set_payload(response, (uint8_t *)message, len);
  LOG_INFO("Received ping request, sent pong response\n");
}

/* --- Main Contiki Process --- */
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

PROCESS_THREAD(sensor_node_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("Starting Sensor Node...\n");

  // Link the handler function to the resource
  res_ping.get_handler = res_get_handler;
  
  // Activate the resource so it can be discovered/accessed by the Gateway
  coap_activate_resource(&res_ping, "ping");

  PROCESS_END();
}