`define BITS_PER_PIXEL (3 * 8)
`define BITS_PER_BLOCK (16 + 48)

module ccc_encoder # (
    parameter WIDTH,    // normally 640
    parameter HEIGHT   // normally 480
) (
    input clk,
    input rst,

    input [WIDTH*HEIGHT*3*8 - 1 : 0] rgb_data,
    output [WIDTH/4*HEIGHT/4*(16+48) - 1 : 0] ccc_data,

    input start,
    output done
);

localparam BITS_PER_PIXEL = `BITS_PER_PIXEL;
localparam BITS_PER_ROW = WIDTH*BITS_PER_PIXEL;

localparam CCC_BITS_PER_BLOCK = `BITS_PER_BLOCK;
localparam CCC_BITS_PER_ROW = WIDTH/4*CCC_BITS_PER_BLOCK;

reg [WIDTH/4*HEIGHT/4 - 1 : 0] blocks_done;

// Assuming (0,0) is top-left in coordinate system
// Move top to bottom, left to right
generate
    genvar block_y;
    genvar block_x;
    for (block_y = 0; block_y < HEIGHT / 4; block_y += 1) begin
        for (block_x = 0; block_x < WIDTH / 4; block_x += 1) begin
            ccc_encoder_4x4 block_encoder (
                .clk(clk),
                .rst(rst),

                .rgb_data({
                    rgb_data[(block_y*4+3)*BITS_PER_ROW+(block_x*4*BITS_PER_PIXEL) +: BITS_PER_PIXEL*4],
                    rgb_data[(block_y*4+2)*BITS_PER_ROW+(block_x*4*BITS_PER_PIXEL) +: BITS_PER_PIXEL*4],
                    rgb_data[(block_y*4+1)*BITS_PER_ROW+(block_x*4*BITS_PER_PIXEL) +: BITS_PER_PIXEL*4],
                    rgb_data[(block_y*4+0)*BITS_PER_ROW+(block_x*4*BITS_PER_PIXEL) +: BITS_PER_PIXEL*4]
                }),
                .ccc_data(
                    ccc_data[block_y*CCC_BITS_PER_ROW+block_x*CCC_BITS_PER_BLOCK +: CCC_BITS_PER_BLOCK]
                ),

                .start(start),
                .done(blocks_done[block_y*WIDTH/4+block_x])
            );
        end
    end
endgenerate

assign done = &blocks_done;

endmodule
