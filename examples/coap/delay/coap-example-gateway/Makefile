CONTIKI_PROJECT = coap-example-gateway
all: $(CONTIKI_PROJECT)

# 1. Optimize CoAP buffers
CFLAGS += -DUIP_CONF_BUFFER_SIZE=140
CFLAGS += -DCOAP_MAX_CHUNK_SIZE=48

# 2. AGGRESSIVE RAM SAVING: Shrink the routing tables to match our small network
CFLAGS += -DUIP_CONF_MAX_ROUTES=5
CFLAGS += -DNBR_TABLE_CONF_MAX_NEIGHBORS=5
CFLAGS += -DCSMA_CONF_MAX_NEIGHBOR_QUEUES=4

MODULES += os/net/app-layer/coap
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

CONTIKI = ../../../..
include $(CONTIKI)/Makefile.include