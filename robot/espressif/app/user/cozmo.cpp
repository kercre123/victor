/** Impelementation for Task 2 cooperative thread
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "task2.h"
#include "client.h"
#include "driver/i2s.h"

void doStuff(void)
{
  os_printf("Look some C++!");
}
