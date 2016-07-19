#include <stdint.h>
extern "C" {
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
}

#include "anki/cozmo/robot/esp.h"

#include "dhTask.h"
#include "bignum.h"
#include "publickeys.h"

#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

static const int RANDOM_BYTES = 16;

enum DiffieHellmanState {
  STATE_IDLE,
  STATE_INIT_POWER_1,
  STATE_POWER_1,
  STATE_INIT_POWER_2,
  STATE_POWER_2,
  STATE_FINISH
};

struct DiffieTask {
  uint8_t remote_exp[RANDOM_BYTES];
  uint8_t local_exp[RANDOM_BYTES];
  
  big_mont_pow_t pow_state;
  big_num_t acc;
};

static DiffieHellmanState state;
static DiffieTask* task;

bool DiffieHellman::Init()  {
  state = STATE_IDLE;
  task = NULL;
  return true;
}

void DiffieHellman::Start(const uint8_t* local, const uint8_t* remote) {
  os_printf("Settings state");

  if (state != STATE_IDLE) {
    return ;
  }

  os_printf("Starting DH");

  /*
  task = reinterpret_cast<DiffieTask*>(os_zalloc(sizeof(DiffieTask)));
  if (task == NULL) {
    os_printf("CANNOT ALLOCATE MEMORY FOR DH TASK");
    return ;
  }

  // Setup the saved values for processing exponent
  memcpy(task->local_exp, local, RANDOM_BYTES);
  memcpy(task->remote_exp, remote, RANDOM_BYTES);

  state = STATE_INIT_POWER_1;
  */
}

static void bytes_to_num(big_num_t& num, const uint8_t* bytes) {
  memcpy(num.digits, bytes, RANDOM_BYTES);
  num.negative = false;
  num.used = RANDOM_BYTES / sizeof(big_num_cell_t);
}

void DiffieHellman::Update(void) {
  using namespace Anki::Cozmo; 

  switch (state) {
  case STATE_INIT_POWER_1:
  {
    big_num_t temp;

    bytes_to_num(temp, task->local_exp);
    mont_power_async_init(task->pow_state, RSA_DIFFIE_MONT, RSA_DIFFIE_EXP_MONT, temp);
    state = STATE_POWER_1;
    break ;
  }
  case STATE_POWER_1:
  {
    if (!mont_power_async(task->pow_state, task->acc)) break ;
    state = STATE_INIT_POWER_2;
    break ;
  }
  case STATE_INIT_POWER_2:
  {
    big_num_t temp;

    bytes_to_num(temp, task->local_exp);
    mont_power_async_init(task->pow_state, RSA_DIFFIE_MONT, task->acc, temp);
    state = STATE_POWER_2;
    break ;
  }
  case STATE_POWER_2:
  {
    if (!mont_power_async(task->pow_state, task->acc)) break ;
    state = STATE_FINISH;
    break ;
  }
  case STATE_FINISH:
  {
    big_num_t temp;
    mont_from(RSA_DIFFIE_MONT, temp, task->acc);
    
    DiffieHellmanResults msg;
    memcpy(msg.result, temp.digits, RANDOM_BYTES);
    RobotInterface::SendMessage(msg);

    os_free(task);
    task = NULL;
    state = STATE_IDLE;
    break ;
  }
  default:
    break ;
  }
}
