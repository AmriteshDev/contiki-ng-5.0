CONTIKI_PROJECT = coap-example-gateway

all: $(CONTIKI_PROJECT)


LDFLAGS += -Wl,--defsym=__stack_size__=640

MODULES += os/net/app-layer/coap

CONTIKI = ../../../..

include $(CONTIKI)/Makefile.include
