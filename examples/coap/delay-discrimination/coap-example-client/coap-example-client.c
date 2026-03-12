// #include "contiki.h"
// #include "coap-engine.h"
// #include "os/sys/log.h"
// #include <string.h>

// #define LOG_MODULE "Sensor"
// #define LOG_LEVEL LOG_LEVEL_INFO

// /* --- Define the CoAP Resource --- */
// // This creates a GET resource at the path "/ping"
// RESOURCE(res_ping,
//          "title=\"Ping Resource\";rt=\"Text\"",
//          NULL, NULL, NULL, NULL);

// // This function is triggered when the Gateway sends a GET request
// static void
// res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
// {
//   const char *message = "pong";
//   size_t len = strlen(message);

//   // Set the payload of the response to "pong"
//   coap_set_payload(response, (uint8_t *)message, len);
//   LOG_INFO("Received ping request, sent pong response\n");
// }

// /* --- Main Contiki Process --- */
// PROCESS(sensor_node_process, "Sensor Node Process");
// AUTOSTART_PROCESSES(&sensor_node_process);

// PROCESS_THREAD(sensor_node_process, ev, data)
// {
//   PROCESS_BEGIN();

//   LOG_INFO("Starting Sensor Node...\n");

//   // Link the handler function to the resource
//   res_ping.get_handler = res_get_handler;
  
//   // Activate the resource so it can be discovered/accessed by the Gateway
//   coap_activate_resource(&res_ping, "ping");

//   PROCESS_END();
// }



#include "contiki.h"
#include "coap-engine.h"
#include "os/sys/log.h"
#include <string.h>
#include <stdio.h>


#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_WARN


/* ---------------- Metric Structure ---------------- */

typedef struct {

  uint16_t queue_metric;
  uint16_t retrans_metric;
  uint16_t loss_metric;
  uint16_t variance_metric;
  uint16_t persistence_metric;
  uint16_t spatial_metric;
  uint16_t link_metric;

} metric_t;

static metric_t metrics;


/* ---------------- Metric Computation ---------------- */

void compute_metrics()
{
  metrics.queue_metric = 20;
  metrics.retrans_metric = 1;
  metrics.loss_metric = 2;
  metrics.variance_metric = 50;
  metrics.persistence_metric = 10;
  metrics.spatial_metric = 10;
  metrics.link_metric = 80;

  LOG_WARN("Metrics computed\n");
}

/* ---------------- Ping Resource ---------------- */

static void res_get_handler(coap_message_t *request,
                            coap_message_t *response,
                            uint8_t *buffer,
                            uint16_t preferred_size,
                            int32_t *offset)
{
  const char *msg = "pong";

  coap_set_payload(response, (uint8_t *)msg, strlen(msg));

  LOG_WARN("Ping received, pong sent\n");
}

RESOURCE(res_ping,
         "title=\"Ping\";rt=\"text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);


/* ---------------- Phase-2 Resource ---------------- */

static void phase2_handler(coap_message_t *request,
                           coap_message_t *response,
                           uint8_t *buffer,
                           uint16_t preferred_size,
                           int32_t *offset)
{
  LOG_WARN("Phase-2 triggered by gateway\n");

  compute_metrics();

  static char msg[120];

snprintf(msg, sizeof(msg),
         "Q=%u,R=%u,P=%u,V=%u,C=%u,S=%u,L=%u",
         metrics.queue_metric,
         metrics.retrans_metric,
         metrics.loss_metric,
         metrics.variance_metric,
         metrics.persistence_metric,
         metrics.spatial_metric,
         metrics.link_metric);

  coap_set_payload(response,(uint8_t*)msg,strlen(msg));
}

RESOURCE(res_phase2,
         "title=\"Phase2\";rt=\"text\"",
         phase2_handler,
         NULL,
         NULL,
         NULL);


/* ---------------- Sensor Process ---------------- */

PROCESS(sensor_process, "Sensor");
AUTOSTART_PROCESSES(&sensor_process);

PROCESS_THREAD(sensor_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_WARN("Sensor ready\n");

  coap_engine_init();

  /* Activate resources */
  coap_activate_resource(&res_ping, "ping");
  coap_activate_resource(&res_phase2, "phase2");

  PROCESS_END();
}