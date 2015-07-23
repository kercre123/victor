`timescale 1ns / 1ps
/// @file Module for using the LEDs on the EVK board for debugging
/// @author Daniel Casner <daniel@anki.com>

module evkLeds (
		output wire [2:0] leds,
		input wire 	  debug1,
		input wire 	  debug2,
		input wire 	  debug3
		);
		
  wire ledcurpu;

  SB_LED_DRV_CUR currentDriver (
    .EN(1'b1),
    .LEDPU(ledcurpu)
  );

  SB_RGB_DRV #(.RGB0_CURRENT("0b000011"), .RGB1_CURRENT("0b000001"), .RGB2_CURRENT("0b000001")) ledDriver
  (
    .RGBLEDEN(1'b1),
    .RGB0PWM(debug1),
    .RGB1PWM(debug2),
    .RGB2PWM(debug3),
    .RGBPU(ledcurpu),
    .RGB0(leds[0]),
    .RGB1(leds[1]),
    .RGB2(leds[2])
  );


endmodule
