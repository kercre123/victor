/* This is a shim to get BLE working
 */

extern "C" {
  #include "app_timer.h"
}

#include <stdint.h>
#include "rtos.h"

extern "C" uint32_t app_timer_create(app_timer_id_t            * p_timer_id,
                          app_timer_mode_t            mode,
                          app_timer_timeout_handler_t timeout_handler)
{
  *p_timer_id = (app_timer_id_t) RTOS::create(timeout_handler, mode == APP_TIMER_MODE_REPEATED);

  return NRF_SUCCESS;
}

extern "C" uint32_t app_timer_start(app_timer_id_t timer_id, uint32_t timeout_ticks, void * p_context)
{
  RTOS::start((RTOS_Task*) timer_id, timeout_ticks, p_context);

  return NRF_SUCCESS;
}

extern "C" uint32_t app_timer_stop(app_timer_id_t timer_id)
{
  RTOS::stop((RTOS_Task*) timer_id);

  return NRF_SUCCESS;
}
