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

#define task2QueueLen 8 ///< Maximum number of task 2 subtasks which can be in the queue
os_event_t task2Queue[task2QueueLen]; ///< Memory for the task 2 queue

static bool i2sStarted;

/// Callback to process incoming radio data
void clientRecvCallback(char* data, unsigned short len)
{
  if (len > I2SPI_MAX_PAYLOAD)
  {
    os_printf("Received data too long for i2s transaction\r\n");
  }
  else
  {
    if (i2sQueueTx((I2SPIPayload*)data, TagAudioData) == false)
    {
      os_printf("Couldn't queue I2SPI tx\r\n");
    }
    if (i2sStarted == false)
    {
      i2sStarted = true;
      i2sStart();
    }
  }
}
/// Callback to process incoming I2SPI data
void i2sRecvCallback(I2SPIPayload* payload, I2SPIPayloadTag tag)
{
  UDPPacket* pkt = clientGetBuffer();
  if (pkt == NULL)
  {
    os_printf("Couldn't get client buffer\r\n");
  }
  os_memcpy(pkt->data, payload, sizeof(I2SPIPayload));
  pkt->len = sizeof(I2SPIPayload);
  clientQueuePacket(pkt);
}

/** Execution of task2 code must return in less than 2 miliseconds
*/
LOCAL void ICACHE_FLASH_ATTR task2Task(os_event_t *event)
{
  // Unpack the arugments
  uint32 sig = event->sig;
  uint32 par = event->par;
  // Whether we're going to repost ourselves
  bool repost = false;

  // Do stuff here


  if (repost)
  {
    task2Post(sig, par);
  }
}

int8_t ICACHE_FLASH_ATTR task2Init(void)
{
  os_printf("Task2 init\r\n");
  if (system_os_task(task2Task, TASK2_PRIO, task2Queue, task2QueueLen) == false)
  {
    os_printf("\tCouldn't register OS task\r\n");
    return -1;
  }
  else
  {
    i2sStarted = false;
    return 0;
  }
}

bool inline task2Post(uint32 signal, uint32 parameter)
{
  return system_os_post(TASK2_PRIO, signal, parameter);
}
