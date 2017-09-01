module crc3
(
  input                          clk,
  input                          reset,

  input                          ctrl_write,
  input      [31:0]              ctrl_writedata,
  input                          ctrl_read,
  output reg [31:0]              ctrl_readdata, 
  input      [1:0]               ctrl_address,
  output reg                     ctrl_waitrequest,  

  output wire                    master_read,
  input  wire [63:0]             master_readdata,
  output reg  [31:0]             master_address,
  input  wire                    master_waitrequest,
  input  wire                    master_readdatavalid,

  output wire                    master_burstcount,
  output wire [7:0]              master_byteenable,
  
  output reg                     irq
//  output reg                     master_reset
);														
																					
											
reg [31:0] start_address;
reg [31:0] counter;
reg [31:0] input_counter;
reg [31:0] acc;
reg [31:0] data;
reg        calc;
reg        update_address;
reg        hi_bit_input_counter_delayed;
reg [31:0] timer;
reg [31:0] timer_to_irq;
reg [31:0] total_time;
reg        timer_to_irq_active; 
reg        ctrl_read_delayed;


assign master_burstcount=1'b1;
assign master_byteenable=8'hFF;

always @(posedge clk)
calc<=master_readdatavalid;

always @(posedge clk, posedge reset)
if(reset)
  start_address<=32'h0000_0000;
else
  if(ctrl_write & (ctrl_address==2'b00))
	 start_address<=ctrl_writedata;

always @(posedge clk)
  update_address<=(ctrl_write & (ctrl_address==2'b00));	 
/*
always @(posedge clk, posedge reset)
if(reset)
  master_read<=1'b0;
else
  if(ctrl_write & ctrl_address)
	 master_read<=1'b1;
  else
    if(counter[31])
      master_read<=1'b0;
*/

assign master_read=~counter[31];		
		
always @(posedge clk)
hi_bit_input_counter_delayed<=input_counter[31];	
	
always @(posedge clk, posedge reset)
if(reset)		
	irq<=1'b0;
else
	irq<= input_counter[31] & ~hi_bit_input_counter_delayed;	

always @(posedge clk, posedge reset)
if(reset)
  timer_to_irq_active<=1'b0;
else
  if(ctrl_write & (ctrl_address==2'b01))	
	 timer_to_irq_active<=1'b1;
  else
	 if(irq)
		timer_to_irq_active<=1'b0;
	 
  
	 
always @(posedge clk, posedge reset)
if(reset)
  counter<=32'hFFFF_FFFF;
else
  if(ctrl_write & (ctrl_address==2'b01))
	 counter<=ctrl_writedata;
  else
    if(master_read & ~master_waitrequest)
      counter<=counter-32'd1;
	 
always @(posedge clk, posedge reset)
if(reset)
  input_counter<=32'hFFFF_FFFF;
else
  if(ctrl_write & (ctrl_address==2'b01))
	 input_counter<=ctrl_writedata;
  else
    if(master_readdatavalid)
      input_counter<=input_counter-32'd1;								
											
always @(posedge clk, posedge reset)
if(reset)
  master_address<=32'h0000_0000;
else
  if(update_address | irq)
    master_address<=start_address;
  else  
    if(master_read & ~master_waitrequest)
      master_address<=master_address +32'd8;  

always @(posedge clk, posedge reset)
if(reset)
  acc<=32'h0000_0000;
else
  if(ctrl_write & (ctrl_address==2'b01))
	 acc<=32'h0000_0000;
  else
    if(calc)    
      acc<=acc+data;

		
always @(posedge clk, posedge reset)
if(reset)
  data<=32'h0000_0000;
else
  if(ctrl_write & (ctrl_address==2'b01))
	 data<=32'h0000_0000;
  else
    if(master_readdatavalid)    
      data<=master_readdata[63:32]+master_readdata[31:0];		

always @(posedge clk)
  ctrl_waitrequest<=1'b0;


always @(posedge clk)
if(irq)
  timer<=32'h0000_0000;
else
  timer<=timer+32'd1;


always @(posedge clk)
if(ctrl_write & (ctrl_address==2'b01))
  timer_to_irq<=32'h0000_0000;
else
  if(timer_to_irq_active)	
    timer_to_irq<=timer_to_irq+32'd1;
	 
	 
always @(posedge clk, posedge reset)
if(reset)
  total_time<=32'h0000_0000;
else
  if(ctrl_read_delayed & (ctrl_address==2'b11))
	 total_time<=32'h0000_0000;
  else
    total_time<=total_time+32'd1;

always @(posedge clk)
  ctrl_read_delayed<=ctrl_read;
	 
always @(posedge clk)
case(ctrl_address) 
  2'b00: ctrl_readdata<=acc;
  2'b01: ctrl_readdata<=timer;
  2'b10: ctrl_readdata<=timer_to_irq;
  2'b11: ctrl_readdata<=total_time;
endcase  

 
  
endmodule