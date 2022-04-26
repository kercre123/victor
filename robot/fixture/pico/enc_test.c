#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/watchdog.h"
#include "pico/binary_info.h"
#include "enc_test.h"

#define VERSION "1.1 2022-04-20"

#define DEBUG_CSV 0

const float conversion_factor = 3.3f / (1 << 8);

// DMA code is adapted from pico-examples/adc/dma_capture provided by the
// Raspberry PI foundation.

#define CAPTURE_CHANNEL 2
#define CAPTURE_DEPTH 600
#define CAPTURE_WIDTH 2
#define CAPTURE_SIZE ( CAPTURE_DEPTH * CAPTURE_WIDTH )

uint8_t capture_buf[CAPTURE_SIZE];

uint init_dma_capture() {
    // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
    adc_init();
    adc_set_round_robin(1 | 2);
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        true     // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(0);

    sleep_ms(1000);
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
        capture_buf,    // dst
        &adc_hw->fifo,  // src
        CAPTURE_SIZE,  // transfer count
        true            // start immediately
    );

    return dma_chan;
}

void run_dma_capture (uint dma_chan) {
    adc_fifo_drain();
    adc_run(true);

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
    adc_fifo_drain();
}

void cleanup_dma_capture(uint dma_chan) {
  //  dma_channel_stop(dma_chan);
  dma_channel_unclaim(dma_chan);
}

void  analyze_capture(float low_voltage, float high_voltage, struct analyze_data* data) {
  analyze_state_t state = ANALYZE_NOT_INITIALIZED;

  data->a_high_tick = -1;
  data->b_high_tick = -1;
  data->a_low_tick = CAPTURE_DEPTH + 1;
  data->b_low_tick = CAPTURE_DEPTH + 1;
  data->a_min_volt = 200;
  data->b_min_volt = 200;
  data->a_max_volt = -1;
  data->b_max_volt = -1;

  for(int i=0;i<CAPTURE_SIZE;i+=CAPTURE_WIDTH) {
    int tick = i / CAPTURE_WIDTH;
    float enc_a = capture_buf[i] * conversion_factor;
    float enc_b = capture_buf[i+1] * conversion_factor;

    // Log Min/max voltages essentially so we can remotely
    // tell if the circuit is working or damaged.
    if (enc_a > data->a_max_volt) data->a_max_volt = enc_a;
    if (enc_b > data->b_max_volt) data->b_max_volt = enc_b;
    if (enc_a < data->a_min_volt) data->a_min_volt = enc_a;
    if (enc_b < data->b_min_volt) data->b_min_volt = enc_b;
    
    if (state == ANALYZE_NOT_INITIALIZED) {
      // Find an area where both encoders aren't seeing anything
      // so we know the rising edge is coming, and we didn't start
      // in the middle of a transition
      if (enc_a < low_voltage && enc_b < low_voltage) {
        state = ANALYZE_INITIALIZED;
      }
    } else if (state == ANALYZE_INITIALIZED) {
      // Find the first time the tick exceeds the high voltage
      // for each encoder to catch the rising edge, then set a flag
      // to stop checking when both are seen
      if (data->a_high_tick < 0 && enc_a > high_voltage) {
        data->a_high_tick = tick;
      }
      if (data->b_high_tick < 0 && enc_b > high_voltage) {
        data->b_high_tick = tick;
      }

      // if we have both high ticks, move on to low
      if (data->a_high_tick > 0 && data->b_high_tick > 0) {
        state = ANALYZE_HIGH_DONE;
      }
    } else if (state == ANALYZE_HIGH_DONE) {

      // Find the first time the tick is smaller than the min voltage
      // for each encoder to catch the falling edge, then set a flag
      // to stop checking when both are seen
      if (data->a_low_tick > CAPTURE_DEPTH && enc_a < low_voltage) {
        data->a_low_tick = tick;
      }
      if (data->b_low_tick > CAPTURE_DEPTH && enc_b < low_voltage) {
        data->b_low_tick = tick;
      }

      // if we have both low edges, call it a day
      if (data->a_low_tick < CAPTURE_DEPTH && data->b_low_tick < CAPTURE_DEPTH) {
        state = ANALYZE_LOW_DONE;
      }
    }
  }

  // ADC clock is 48 Mhz, and it takes 96 clock cycles to capture a reading.
  // This gets us 500,000 samples per second when DMA is on full blast.
  // Taking A then B samples gets us to 250,000 sample sessions per second.
  // 1000000 uSec / 250000 sample sessions
  const int usec_per_sample_session = 4; 
    
  // Calculate the deltas. B always trails.
  data->high_delta_us = (data->a_high_tick - data->b_high_tick) *
    usec_per_sample_session;
  data->low_delta_us = (data->a_low_tick - data->b_low_tick) *
    usec_per_sample_session;
}

void print_capture_data(float low_voltage, float high_voltage, struct analyze_data* data) {
  printf("[%f V - %f V]: HIGH DELTA %d uS | LOW_DELTA %d uS | Av (min: %1.2f, max: %1.2f), Bv (min: %1.2f, max: %1.2f)\n",
         low_voltage, high_voltage,
         data->high_delta_us, data->low_delta_us,
	 data->a_min_volt, data->a_max_volt,
	 data->b_min_volt, data->b_max_volt
	 );
}

test_status_t single_test(float low_voltage, float high_voltage, struct analyze_data* data) {
  watchdog_update();
  
  uint dma_chan = init_dma_capture();
  run_dma_capture(dma_chan);
  cleanup_dma_capture(dma_chan);

  if (DEBUG_CSV){
    puts("=== BEGIN ===\n");
    
    for(int i=0;i<CAPTURE_SIZE;i+=CAPTURE_WIDTH) {
      float enc_a = capture_buf[i+1] * conversion_factor;
      float enc_b = capture_buf[i] * conversion_factor;
      if (DEBUG_CSV) printf("%f,%f\n", enc_a, enc_b);
    }
    if (DEBUG_CSV) puts("=== END ===\n");
  }

  analyze_capture(low_voltage, high_voltage, data);
  print_capture_data(low_voltage, high_voltage, data);
  
  if (data->a_high_tick < 0 || data->b_high_tick < 0) {
    return ERROR_NO_HIGH;
  } else if (data->a_low_tick > CAPTURE_DEPTH ||
             data->b_low_tick > CAPTURE_DEPTH) {
    return ERROR_NO_LOW;
  } else if (data->high_delta_us < -1 || data->low_delta_us < -1) {
    return ERROR_WIRES_REVERSED;
  } else if (data->high_delta_us < DELTA_LOW || data->low_delta_us < DELTA_LOW) {
    return ERROR_DELTA_TOO_LOW;
  } else if (data->high_delta_us > DELTA_HIGH || data->low_delta_us > DELTA_HIGH) {
    return ERROR_DELTA_TOO_HIGH;
  }

  return SUCCESS;
}

#define TEST_COUNT 5

float tests[TEST_COUNT][2] = {
  {1.1,1.6},
  {1.0,1.7},
  {0.9,1.8},
  {0.8,1.9},
  {0.7,2.0},
};

test_status_t test_encoder_only() {
  // Run adc capture
  float *test_params;

  float low_voltage = 1.1;
  float high_voltage = 1.6;
  
  struct analyze_data data;

  // Disable LED, make sure we don't get anything good
  gpio_put(ENC_LED_PIN, 1);

  single_test(low_voltage, high_voltage, &data);
  print_capture_data(low_voltage, high_voltage, &data);

  if (data.a_max_volt > 0.2 || data.b_max_volt > 0.2) {
    printf("ENCODER ONLY: VOLTAGE TOO HIGH WHEN LED IS OFF");
    return ERROR_BAD_LED_OFF;
  }
  
  // Re-enable LED
  gpio_put(ENC_LED_PIN, 0);

  single_test(low_voltage, high_voltage, &data);
  print_capture_data(low_voltage, high_voltage, &data);

  if (data.a_min_volt < 3 || data.b_min_volt < 3) {
    printf("ENCODER ONLY: VOLTAGE TOO LOW WHEN LED IS ON");
    return ERROR_BAD_LED_ON;
  }
  
  return SUCCESS;
}
test_status_t test_encoder_with_gearbox() {
  // Run adc capture
  float *test_params;
  test_status_t result;

  struct analyze_data data;

  // Disable LED, make sure we don't get anything good
  gpio_put(ENC_LED_PIN, 1);

  result = single_test(1.1, 1.6, &data);
  if(result != ERROR_NO_HIGH) {
    puts("ERROR_LED_SHOULD_BE_OFF");
    return ERROR_LED_SHOULD_BE_OFF;
  }

  // Re-enable LED
  gpio_put(ENC_LED_PIN, 0);

  for(int i=0;i<TEST_COUNT;i++) {
    test_params = tests[i];
    
    result = single_test(test_params[0], test_params[1], &data);
    if (result != SUCCESS) { break; }
  }

  if (result == SUCCESS) {
    puts("OK");
  } else {
    printf("%s", "FAILED: ");
    switch(result) {
    case SUCCESS:
      puts("SUCCESS??");
      break;
    case ERROR_NO_HIGH:
      puts("NO HIGH");
      break;
    case ERROR_NO_LOW:
      puts("NO LOW");
      break;
    case ERROR_DELTA_TOO_LOW:
      puts("LOW DELTA");
      break;
    case ERROR_DELTA_TOO_HIGH:
      puts("HIGH DELTA");
      break;
    case ERROR_WIRES_REVERSED:
      puts("WIRES REVERSED");
      break;
    default:
      puts("UNKNOWN ERROR");
    }
  }

  return result;
}

bool encoder_detected(){
  adc_init();
  adc_select_input(2);
  float voltage = (adc_read() >> 4) * conversion_factor; // make it 8 bit
  return voltage > 1.5;
}

void wait_for_encoder(bool detected) {
  int wait_counter=0;

  if (detected) {
    puts("");
    puts("=======================");
    puts("VECTOR ENCODER TESTER");
    printf("VERSION %s\n", VERSION);
    puts("=======================");
    puts("");
  }
  printf("\nWaiting for %s encoder", detected ? "detected" : "undetected");
  
  while(encoder_detected() != detected) {
    printf("%s", ".");
    wait_counter++;
    if (wait_counter % 75 == 0) { puts("");}
    watchdog_update();
    sleep_ms(250);
  }
  puts("");
}

void set_led_color(color_t color) {
  switch (color) {
  case RED:
    gpio_put(LED_RED_PIN,1);
    gpio_put(LED_GREEN_PIN,0);
    gpio_put(LED_BLUE_PIN,0);
    break;
  case GREEN:
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,1);
    gpio_put(LED_BLUE_PIN,0);
    break;
  case BLUE:
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,0);
    gpio_put(LED_BLUE_PIN,1);
    break;
  case WHITE:
    gpio_put(LED_RED_PIN,1);
    gpio_put(LED_GREEN_PIN,1);
    gpio_put(LED_BLUE_PIN,1);
    break;
  case YELLOW:
    gpio_put(LED_RED_PIN,1);
    gpio_put(LED_GREEN_PIN,1);
    gpio_put(LED_BLUE_PIN,0);
    break;
  case MAGENTA:
    gpio_put(LED_RED_PIN,1);
    gpio_put(LED_GREEN_PIN,0);
    gpio_put(LED_BLUE_PIN,1);
    break;
  case CYAN:
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,1);
    gpio_put(LED_BLUE_PIN,1);
    break;
  case OFF:
  default:
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,0);
    gpio_put(LED_BLUE_PIN,0);
    break;
  }
}

void setup_gpio() {
  gpio_init(ONBOARD_LED_PIN);
  gpio_set_dir(ONBOARD_LED_PIN, GPIO_OUT);
  gpio_put(ONBOARD_LED_PIN, 0);
  
  adc_gpio_init(DETECT_PIN);
  adc_gpio_init(ENC_A_PIN);
  adc_gpio_init(ENC_B_PIN);

  gpio_init(ENC_LED_PIN);
  gpio_set_dir(ENC_LED_PIN, GPIO_OUT);
  gpio_put(ENC_LED_PIN, 0);

  gpio_init(MOTOR_ENABLE_PIN);
  gpio_set_dir(MOTOR_ENABLE_PIN, GPIO_OUT);
  gpio_put(MOTOR_ENABLE_PIN, 0);

  gpio_init(LED_RED_PIN);
  gpio_set_dir(LED_RED_PIN, GPIO_OUT);
  gpio_put(LED_RED_PIN, 0);

  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_put(LED_GREEN_PIN, 0);

  gpio_init(LED_BLUE_PIN);
  gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
  gpio_put(LED_BLUE_PIN, 0);

  // Currently GPIO 3 so that we can jump the
  // middle two pins on either J1 or J6 to enable
  // this mode
  gpio_init(MODE_PIN);
  gpio_set_dir(MODE_PIN, GPIO_IN);
  gpio_set_pulls(MODE_PIN, false, true);
}

int main() {
  bi_decl(bi_program_description("Encoder Tester"));
  bi_decl(bi_1pin_with_name(ONBOARD_LED_PIN, "On-board LED"));

  stdio_init_all();

  if (watchdog_caused_reboot()) {
  }

  // Five seconds. Tests are a bit time consuming.
  watchdog_enable(5000,1);

  setup_gpio();

  color_t blink = GREEN;
  
  // Show watchdog error when we're not watching the USB log
  if (watchdog_caused_reboot()) {
    blink = RED;
  }

  // Blink green if happy, red if we're rebooting
  for (int i=0;i<(blink == RED ? 10 : 3);i++) {
    set_led_color(blink);
    sleep_ms(100);
    set_led_color(OFF);
    sleep_ms(100);
  }
  
  // Show watchdog error when we're not watching the USB log
  if (watchdog_caused_reboot()) {
    puts("");
    puts("ERROR: WATCHDOG TIMEOUT ERROR!");
    puts("ERROR: WATCHDOG FORCED REBOOT!");
  }
  
  while (1) {
    gpio_put(ONBOARD_LED_PIN, 1);
    
    set_led_color(WHITE);

    wait_for_encoder(true);
    // start motor, give it 200 ms to get up to speed
    gpio_put(MOTOR_ENABLE_PIN, 1);
    sleep_ms(200);

    set_led_color(BLUE);

    test_status_t result;

    if (gpio_get(MODE_PIN)) {
      printf("\n%s\n\n", "ENCODER ONLY TEST");
      result = test_encoder_only();
    } else {
      printf("\n%s\n\n", "GEARBOX TEST");
      result = test_encoder_with_gearbox();
    }
    
    if(result == SUCCESS) {
      set_led_color(GREEN);
    } else {
      set_led_color(RED);
    }

    // stop the motor
    gpio_put(MOTOR_ENABLE_PIN, 0);

    wait_for_encoder(false);
    set_led_color(OFF);
  }

}
