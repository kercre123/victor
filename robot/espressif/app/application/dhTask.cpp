#include <stdint.h>

#include "anki/cozmo/robot/esp.h"

#include "dhTask.h"
#include "bignum.h"
#include "publickeys.h"

#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

enum DiffieHellmanState {
  STATE_IDLE,
  STATE_INIT_POWER_1,
  STATE_POWER_1,
  STATE_INIT_POWER_2,
  STATE_POWER_2,
  STATE_FINISH
};

static const int RANDOM_BYTES = 16;

static big_num_t remote_exp;
static big_num_t local_exp;
static DiffieHellmanState state;

void DiffieHellman::Init(uint8_t local[], uint8_t remote[]) {
  // Newest clears oldest
  memcpy(local_exp.digits, local, RANDOM_BYTES);
  memcpy(remote_exp.digits, remote, RANDOM_BYTES);

  local_exp.used = RANDOM_BYTES / sizeof(big_num_cell_t);
  remote_exp.used = RANDOM_BYTES / sizeof(big_num_cell_t);

  local_exp.negative = false;
  remote_exp.negative = false;

  state = STATE_INIT_POWER_1;
}

void DiffieHellman::Update(void) {
  using namespace Anki::Cozmo; 

  static big_mont_pow_t pow_state;
  static big_num_t acc;

  switch (state) {
  case STATE_INIT_POWER_1:
    mont_power_async_init(pow_state, RSA_DIFFIE_MONT, RSA_DIFFIE_EXP_MONT, local_exp);
    state = STATE_POWER_1;
    break ;
  case STATE_POWER_1:
    if (!mont_power_async(pow_state, acc)) break ;
    state = STATE_INIT_POWER_2;
    break ;
  case STATE_INIT_POWER_2:
    mont_power_async_init(pow_state, RSA_DIFFIE_MONT, acc, remote_exp);
    state = STATE_POWER_1;
    break ;
  case STATE_POWER_2:
    if (!mont_power_async(pow_state, acc)) break ;
    state = STATE_FINISH;
    break ;
  case STATE_FINISH:
    big_num_t result;
    mont_from(RSA_DIFFIE_MONT, result, acc);
    
    DiffieHellmanResults msg;
    memcpy(msg.result, result.digits, RANDOM_BYTES);
    RobotInterface::SendMessage(msg);

    state = STATE_IDLE;
    break ;
  case STATE_IDLE:
    break ;
  }
}
