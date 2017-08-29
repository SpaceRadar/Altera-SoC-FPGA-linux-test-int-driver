module DMAMaster
(
  input                          clk,
  input                          reset,

  input                          ctrl_write,
  input      [31:0]              ctrl_writedata,
  input                          ctrl_read,
  output reg [31:0]              ctrl_readdata, 
  input                          ctrl_address,
  output reg                     ctrl_waitrequest,  

  output reg                     master_write,
  output reg [63:0]              master_writedata,
  output reg [31:0]              master_address,
  input  wire                    master_waitrequest,
  output wire                    master_burstcount,
  output wire [7:0]              master_byteenable,
  
  output reg                     irq
//  output reg                     master_reset
);														
																					
											
reg [31:0] start_address;
reg [31:0] counter;
reg        write_delayed;


assign master_burstcount=1'b1;
assign master_byteenable=8'hFF;


always @(posedge clk, posedge reset)
if(reset)
  start_address<=32'h0000_0000;
else
  if(ctrl_write & !ctrl_address)
	 start_address<=ctrl_writedata;


always @(posedge clk, posedge reset)
if(reset)
  master_write<=1'b0;
else
  if(ctrl_write & ctrl_address)
	 master_write<=1'b1;
  else
    if(counter[31])
      master_write<=1'b0;
		
		
always @(posedge clk, posedge reset)
if(reset)		
	write_delayed<=1'b0;
else
	write_delayed<=master_write;

	
always @(posedge clk, posedge reset)
if(reset)		
	irq<=1'b0;
else
	irq<= write_delayed & (~master_write);	

	 
always @(posedge clk, posedge reset)
if(reset)
  counter<=32'h0000_0000;
else
  if(ctrl_write & ctrl_address)
	 counter<=ctrl_writedata;
  else
    if(~master_waitrequest)
      counter<=counter-master_write;
	 
								
											
always @(posedge clk, posedge reset)
if(reset)
  master_address<=32'h0000_0000;
else
  if(ctrl_write & !ctrl_address)
    master_address<=ctrl_writedata;
  else  
    if(master_write & ~master_waitrequest)
      master_address<=master_address +32'd8;  

always @(posedge clk, posedge reset)
if(reset)
  master_writedata<=64'h0000_0000_0000_0000;
else
  if(ctrl_write & ctrl_address)
	 master_writedata<=64'h0000_0000_0000_0000;
  else
    if(master_write & ~master_waitrequest)    
      master_writedata<=master_writedata+master_write;


always @(posedge clk, posedge reset)
if(reset)
  ctrl_waitrequest<=1'b0;
  
always @(posedge clk, posedge reset)
if(reset)
  ctrl_readdata<=32'h0000_0000;
  
  
endmodule