// Driver and load in different leaf modules, connected through a parent
// bus bit. Buffer insertion on that net punches hierarchical ports whose
// names derive from the flat net base name "mid[5]".
module leaf_drv (
  input clk,
  input in1,
  output out1
);

  DFF_X1 dff1 (.D(in1), .CK(clk), .Q(out1));

endmodule

module leaf_load (
  input clk,
  input in1,
  output out1
);

  wire n1, n2;
  BUF_X1 buf1 (.A(in1), .Z(n1));
  BUF_X1 buf2 (.A(in1), .Z(n2));
  AND2_X1 and1 (.A1(n1), .A2(n2), .ZN(out1));

endmodule

module top (
  input clk,
  input [7:0] in,
  output [7:0] out
);

  wire [7:0] mid;

  leaf_drv u_drv (.clk(clk), .in1(in[5]), .out1(mid[5]));
  leaf_load u_load1 (.clk(clk), .in1(mid[5]), .out1(out[5]));
  leaf_load u_load2 (.clk(clk), .in1(mid[5]), .out1(out[6]));

endmodule
