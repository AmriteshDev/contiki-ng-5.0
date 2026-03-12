#include "contiki.h"
#include "coap-engine.h"
#include "coap-separate.h"
#include "sys/etimer.h"
#include "os/sys/log.h"

#define LOG_MODULE "Malicious"
#define LOG_LEVEL LOG_LEVEL_WARN

/* Store the request context while we intentionally hold the packet */
static coap_separate_t request_metadata;
static struct etimer delay_timer;

/* Declare the background process */
PROCESS(malicious_node_process, "Malicious Node Process");
AUTOSTART_PROCESSES(&malicious_node_process);

/* 1. Immediate Request Handler */
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  LOG_WARN("Intercepted ping. Holding packet...\n");
  
  /* Accept the request, tell the CoAP stack we will reply later */
  coap_separate_accept(request, &request_metadata);
  
  /* Start a timer to intentionally delay the response by 800 ms. 
     This simulates an attacker holding the packet to induce bursty delays. */
  etimer_set(&delay_timer, (CLOCK_SECOND * 800) / 1000);
  
  /* Wake up the background process to handle the delayed sending */
  process_poll(&malicious_node_process);
}

/* Link the handler to the standard /ping resource */
RESOURCE(res_ping, "title=\"Malicious Ping\";rt=\"Text\"", res_get_handler, NULL, NULL, NULL);

/* 2. Background Process to send the delayed response */
PROCESS_THREAD(malicious_node_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_WARN("Starting Malicious Sensor Node...\n");
  coap_activate_resource(&res_ping, "ping");

  while(1) {
    PROCESS_YIELD();

    /* If the timer expires after being polled by the handler */
    if(ev == PROCESS_EVENT_TIMER && data == &delay_timer) {
      LOG_WARN("Delay complete. Releasing pong response to victim...\n");

      /* Formulate the delayed "pong" response using the updated Contiki-NG macro */
      coap_message_t response[1];
      coap_init_message(response, COAP_TYPE_CON, CONTENT_2_05, 0);
      coap_set_payload(response, (uint8_t *)"pong", 4);

      /* Resume and send the packet back to the Gateway */
      coap_separate_resume(response, &request_metadata, CONTENT_2_05);
    }
  }

  PROCESS_END();
}