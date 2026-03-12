#include "contiki.h"
#include "coap-engine.h"
#include "os/sys/log.h"
#include "sys/clock.h"

#define LOG_MODULE "Malicious"
#define LOG_LEVEL LOG_LEVEL_WARN


/* -------- Ping Resource -------- */

static void
res_get_handler(coap_message_t *request,
                coap_message_t *response,
                uint8_t *buffer,
                uint16_t preferred_size,
                int32_t *offset)
{

  LOG_WARN("Ping received - introducing delay\n");

  /* Malicious delay (3 seconds) */

  clock_wait(3 * CLOCK_SECOND);

  LOG_WARN("Delayed pong sent\n");

  const char *msg = "pong";

  coap_set_payload(response, msg, strlen(msg));
}


/* Register resource */

RESOURCE(res_ping,
         "title=\"Ping\";rt=\"Delay\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);



/* -------- Process -------- */

PROCESS(malicious_node_process, "Malicious Node");
AUTOSTART_PROCESSES(&malicious_node_process);


PROCESS_THREAD(malicious_node_process, ev, data)
{

  PROCESS_BEGIN();

  coap_engine_init();

  coap_activate_resource(&res_ping, "ping");

  LOG_WARN("Malicious sensor ready\n");

  PROCESS_END();
}