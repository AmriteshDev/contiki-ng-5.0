#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define LOG_LEVEL_APP LOG_LEVEL_INFO

// Set the max CoAP message size (using the updated macro name for newer Contiki-NG)
#define COAP_MAX_CHUNK_SIZE 64

// INCREASE the IPv6 buffer size so the Z1 mote can hold the full CoAP packet
#define UIP_CONF_BUFFER_SIZE 240

// Enable floating point support for Phase 1 math
#define PRINTF_FORMAT_FLOATING_POINT 1

#endif /* PROJECT_CONF_H_ */