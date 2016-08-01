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
  STATE_RINV,
  STATE_MODULO
};

struct DiffieTask {
  uint8_t remote_exp[RANDOM_BYTES];
  
  // None of these values are used at the same time
  union {
    uint8_t local_exp[RANDOM_BYTES];
    big_mont_pow_t pow_state;
    big_modulo_t mod_state;
  };
};

static DiffieHellmanState state;
static DiffieTask* task;

bool DiffieHellman::Init()  {
  state = STATE_IDLE;
  task = NULL;
  return true;
}

void DiffieHellman::Start(const uint8_t* local, const uint8_t* remote) {
  // Restart DH (pairing reset)
  if (state != STATE_IDLE) {
    os_free(task);
    state = STATE_IDLE;
  }

  task = reinterpret_cast<DiffieTask*>(os_zalloc(sizeof(DiffieTask)));

  if (task == NULL) {
    os_printf("DH Allocation Failed");
    return ;
  }

  os_printf("DH Start");

  memcpy(task->local_exp, local, RANDOM_BYTES);
  memcpy(task->remote_exp, remote, RANDOM_BYTES);
  
  state = STATE_INIT_POWER_1;
}

static void bytes_to_num(big_num_t& num, const uint8_t* bytes) {
  memcpy(num.digits, bytes, RANDOM_BYTES);
  num.negative = false;
  num.used = RANDOM_BYTES / sizeof(big_num_cell_t);
}

void DiffieHellman::Update(void) {
  using namespace Anki::Cozmo; 

  if (state == STATE_IDLE) return ;
  
  switch (state) {
  case STATE_INIT_POWER_1:
  {
    big_num_t temp;

    bytes_to_num(temp, task->local_exp);
    mont_power_async_init(RSA_DIFFIE_MONT, task->pow_state, RSA_DIFFIE_EXP_MONT, temp);
    state = STATE_POWER_1;
    break ;
  }
  case STATE_POWER_1:
  {
    if (!mont_power_async(RSA_DIFFIE_MONT, task->pow_state)) break ;
    state = STATE_INIT_POWER_2;
    break ;
  }
  case STATE_INIT_POWER_2:
  {
    big_num_t temp;

    bytes_to_num(temp, task->remote_exp);
    mont_power_async_init(RSA_DIFFIE_MONT, task->pow_state, task->pow_state.result, temp);
    state = STATE_POWER_2;
    break ;
  }
  case STATE_POWER_2:
  {
    if (!mont_power_async(RSA_DIFFIE_MONT, task->pow_state)) break ;
    
    state = STATE_RINV;
    break ;
  }
  case STATE_RINV:
  {
    big_num_t temp;
        
    // This is mont_from inlined
    {
      big_num_t mont_rinv = RSA_DIFFIE_MONT.rinv;    
      big_multiply(temp, task->pow_state.result, mont_rinv);
    }
    // Setup modulo
    {
      big_num_t mont_modulo = RSA_DIFFIE_MONT.modulo;
      big_modulo_async_init(task->mod_state, temp, mont_modulo);
    }

    state = STATE_MODULO;
    break ;
  }
  case STATE_MODULO:
  {
    if (!big_modulo_async(task->mod_state)) break ;

    DiffieHellmanResults msg;
    memcpy(msg.result, task->mod_state.modulo.digits, RANDOM_BYTES);
    RobotInterface::SendMessage(msg);

    os_printf("DH done");

    os_free(task);
    task = NULL;
    state = STATE_IDLE;
    break ;
  }

  default:
    break ;
  }
}
