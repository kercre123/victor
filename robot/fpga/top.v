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

endmodule
