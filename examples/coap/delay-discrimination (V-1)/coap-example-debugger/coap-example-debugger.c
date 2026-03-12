#include "contiki.h"
#include "net/routing/routing.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "sys/log.h"
#include "sys/etimer.h"

#define LOG_MODULE "Node"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(debug_process, "Node Debug");
AUTOSTART_PROCESSES(&debug_process);

PROCESS_THREAD(debug_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  etimer_set(&timer, 20 * CLOCK_SECOND);

  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("\n---- Node status ----\n");

    /* Print neighbor table */
    LOG_INFO("Neighbors:\n");

    uip_ds6_nbr_t *nbr;

    for(nbr = uip_ds6_nbr_head();
        nbr != NULL;
        nbr = uip_ds6_nbr_next(nbr)) {

      LOG_INFO("Neighbor: ");
      LOG_INFO_6ADDR(&nbr->ipaddr);
      LOG_INFO_("\n");
    }

    /* Print parent (next hop to root) */

    rpl_dag_t *dag = rpl_get_any_dag();

    if(dag != NULL && dag->preferred_parent != NULL) {

      LOG_INFO("Preferred parent: ");

      LOG_INFO_6ADDR(
      rpl_parent_get_ipaddr(dag->preferred_parent));

      LOG_INFO_("\n");
    }

    etimer_reset(&timer);
  }

  PROCESS_END();
}