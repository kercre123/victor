`timescale 1ns / 1ps
/// @file FPGA image signal processing and compression code. Top file for the FPGA firmware
/// @author Daniel Casner <daniel@anki.com>


module top (
  // Pins
  inout wire sio,  ///< SPI data in and data out in SIO mode
  input wire sck,  ///< SPI clock
  input wire scsn, ///< SPI chip select, active low
  input wire xclk, ///< Camera clock input (12MHz?)
  input wire pclk, ///< Camera pixel clock (>12MHz)
  input wire [5:0] camD; ///< Camera pixel data
  );

//////////////// SPI Testing /////////////////////

  reg inNOut; ///< SIO direction is in / not out
  reg [31:0] testFifo;

  initial begin
    sio <= 1'bz;
    inNOut <= 1'b1;
    testFifo <= 32'h00000000;
  end

  always @(posedge scsn) begin
    sio    <= 1'bz; // Make SIO an input
    inNOut <= ! inNOut; // Reverse direction for next transaction
  end

  always @(posedge sclk and scsn == 1'b0) begin
    if (inNOut == 1'b1) begin // Receiving data
      testFifo <= {testFifo[30:0], sio};
    end
    else begin // Transmistting data
      sio <= testFifo[31];
      testFifo <= {testFifo[30:0], 0};
    end
  end

///////// Camera buffering ////////////////////////////////////////////////////

  /// Camera timing signals
  wire blankCounterReset; ///< Hold the blank counters in reset
  wire hBlankStrobe; ///< Strobe signal when h blank is detected
  wire vBlankStrobe; ///< Strobe signal when v blank is detected

  /// Wire these ^^ into LFSR counters here

  /// Block ram control signals
  wire [7:0] ebrRData;
  reg  [8:0] ebrRAddr;
  wire       ebrRClk;
  reg        ebrRE;
  wire [7:0] ebrWData;
  reg  [8:0] ebrWAddr;
  wire       ebrWClk;
  reg        ebrWE;

  SB_RAM512x8 ebr0 (
    .RDATA(ebrRData),
    .RADDR(ebrRAddr),
    .RCLK(ebrRClk),
    .RCLKE(ebrRE),
    .RE(ebrRE),
    .WADDR(ebrWAddr),
    .WCLK(ebrWClk),
    .WCLKE(ebrWE),
    .WDATA(ebrWData),
    .WE(ebrWE)
  );
  defparam ebr0.INIT_0 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_1 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_2 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_3 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_4 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_5 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_6 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_7 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_8 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_9 = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_A = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_B = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_C = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_D = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_E = 256'h0000000000000000000000000000000000000000000000000000000000000000;
  defparam ebr0.INIT_F = 256'h0000000000000000000000000000000000000000000000000000000000000000;






endmodule
