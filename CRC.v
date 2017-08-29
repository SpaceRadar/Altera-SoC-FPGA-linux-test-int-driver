module CRC
(
  input                          clk,
  input                          reset,

  input  wire [9:0]              address,  
  input  wire                    write,
  input  wire [63:0]             writedata,
  input  wire                    read,
  output reg [63:0]              readdata,
  
  output reg                     irq
  
);														
																					

reg [31:0] acc;											
reg [31:0] data_st1;
reg        write_st1;
reg [31:0] counter;
reg        crc_state;


always @(posedge clk)
if(reset)
	counter<=32'h0000_0000;
else
	if(write)
		if(address[9])
			counter<=writedata[31:0];
		else
			counter<=counter-32'h1;

always @(posedge clk)
if(reset)
	crc_state<=1'b0;
else
	if(write & address[9])
		crc_state<=1'b1;
	else
		if(counter==32'h0000_0000)
			crc_state<=1'b0;


always @(posedge clk)
if(reset)
	irq<=1'b0;
else
	irq<=(counter==32'h0000_0000) & crc_state;	
			
always @(posedge clk)
if(write)
	if(address[9])
		data_st1<=32'h0000_0000;
	else
		data_st1<=writedata[63:32]+writedata[31:0];

		
always @(posedge clk)
if(reset)
	acc<=32'h0000_0000;
else	
	if(write_st1)
		if(address[9])
			acc<=32'h0000_0000;
		else
			acc<=acc+data_st1;


always @(posedge clk)
write_st1<=write;
		
always @(posedge clk)
readdata<={32'h0,acc};

  
  
endmodule