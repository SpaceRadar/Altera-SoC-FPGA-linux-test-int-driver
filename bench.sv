`timescale 1ns / 100ps
module bench;

localparam DATA_WIDTH = 22;
localparam TWID_WIDTH = 16;
localparam FFT_SIZE   = 8;
localparam BUTT_COUNT = 2;
localparam STRIDE_WIDTH  = 4; 

reg reset, clk;
reg ctrl_write,ctrl_read;
reg [31:0] ctrl_writedata, ctrl_address;

reg [63:0] master_readdata;
reg        master_waitrequest,master_readdatavalid;

crc3 crc3_inst(
                           .reset(reset),
                           .clk(clk),
                           .ctrl_write(ctrl_write),
                           .ctrl_writedata(ctrl_writedata),
                           .ctrl_read(ctrl_read),
//                           .ctrl_readdata(ctrl_readdata), 
                           .ctrl_address(ctrl_address),
//  output reg                     ctrl_waitrequest,  

//  output reg                     master_read,
  			   .master_readdata(master_readdata),
//  output reg  [31:0]             master_address,
                           .master_waitrequest(master_waitrequest),
                           .master_readdatavalid(master_readdatavalid)

//  output wire                    master_burstcount,
//  output wire [7:0]              master_byteenable,
  
//  output reg                     irq
//  output reg                     master_reset
                           );

always
  #10 clk = ~clk;

initial
begin
  clk = 1;
  reset = 0;
  ctrl_write =0;
  ctrl_read=0;
  ctrl_address=0;
  master_readdata=0;
  master_waitrequest=0;
  master_readdatavalid=0;

  #80 reset =1;
  #20 reset =0;


   
             
end 

endmodule
