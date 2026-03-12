#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define UIP_CONF_BUFFER_SIZE 220
#define COAP_MAX_CHUNK_SIZE 48

#define STACK_CHECK_CONF_ENABLED 0

#define LOG_CONF_LEVEL_RPL LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_WARN

#define LOG_CONF_LEVEL_GATEWAY LOG_LEVEL_INFO
// ```

// And tell me: **what is your Cooja simulation topology?** Are all 5 sensor nodes in direct radio range of the gateway (node 1), or are some multi-hop? This determines whether the neighbor cache approach works at all.

// Also run this quick check — in your gateway code, what does `uip_ip6addr` produce? The link-layer address shows `c10c.0000.0000.000X` but the IPv6 shows `c30c` — note the `3` vs `1`. Let me verify the IID mapping:
// ```
// Link-layer: c1:0c:00:00:00:00:00:02
// fe80 IID:   c30c:0000:0000:0002     ← bit-flip of first byte (EUI-64 universal bit)
// fd00 target: fd00::c30c:0:0:2      ← this is correct

#endif