
CONTIKI_PROJECT = coap-example-client
all: $(CONTIKI_PROJECT)

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

# Enable RPL (Routing Protocol for Low-Power and Lossy Networks) for multihop
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

CONTIKI = ../../../..
include $(CONTIKI)/Makefile.include