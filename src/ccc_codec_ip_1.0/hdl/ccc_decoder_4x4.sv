`define BITS_PER_PIXEL (3 * 8)
`define BITS_PER_BLOCK (16 + 48)

module ccc_decoder_4x4 (
    input clk,
    (* mark_debug = "true" *) input rst,

    (* mark_debug = "true" *) input [`BITS_PER_BLOCK - 1 : 0] ccc_data,
    (* mark_debug = "true" *) output [4*4*`BITS_PER_PIXEL - 1 : 0] rgb_data,
    
    (* mark_debug = "true" *) input start,
    (* mark_debug = "true" *) output reg done
);

(* mark_debug = "true" *) reg [`BITS_PER_PIXEL - 1 : 0] rgb_data_up [0:3] [0:3];
(* mark_debug = "true" *) reg [`BITS_PER_BLOCK - 1 : 0] ccc_data_up;
generate
    genvar y, x;
    for (y = 0; y < 4; y += 1) begin
        for (x = 0; x < 4; x += 1) begin
            assign rgb_data[(y*4+x)*`BITS_PER_PIXEL +: `BITS_PER_PIXEL] = rgb_data_up[y][x];
        end
    end
endgenerate
assign ccc_data_up = ccc_data;

(* mark_debug = "true" *) reg [`BITS_PER_PIXEL - 1 : 0] rgb_data_up_next [0:3] [0:3];

typedef enum {
    IDLE,
    DECODE
} STATE;

(* mark_debug = "true" *) STATE state;
(* mark_debug = "true" *) STATE state_next;

(* mark_debug = "true" *) reg done_next;

(* mark_debug = "true" *) reg [16 - 1 : 0] bitmap;

(* mark_debug = "true" *) reg [8 - 1 : 0] bucket_0_avg_r;
(* mark_debug = "true" *) reg [8 - 1 : 0] bucket_0_avg_g;
(* mark_debug = "true" *) reg [8 - 1 : 0] bucket_0_avg_b;
(* mark_debug = "true" *) reg [8 - 1 : 0] bucket_1_avg_r;
(* mark_debug = "true" *) reg [8 - 1 : 0] bucket_1_avg_g;
(* mark_debug = "true" *) reg [8 - 1 : 0] bucket_1_avg_b;

assign bitmap           = ccc_data[48 +: 16];
assign bucket_0_avg_r   = ccc_data[40 +: 08];
assign bucket_0_avg_g   = ccc_data[32 +: 08];
assign bucket_0_avg_b   = ccc_data[24 +: 08];
assign bucket_1_avg_r   = ccc_data[16 +: 08];
assign bucket_1_avg_g   = ccc_data[08 +: 08];
assign bucket_1_avg_b   = ccc_data[00 +: 08];

integer rgb_y, rgb_x;

always_ff @(posedge clk) begin
    if (rst) begin
        state <= IDLE;
        done <= 0;
        
        for (rgb_y = 0; rgb_y < 4; rgb_y += 1) begin
            for (rgb_x = 0; rgb_x < 4; rgb_x += 1) begin
                rgb_data_up[rgb_y][rgb_x] <= 0;
            end
        end
    end
    else begin
        case (state)
            IDLE: begin
                // Make sure to reset these values before decoding a new frame
                for (rgb_y = 0; rgb_y < 4; rgb_y += 1) begin
                    for (rgb_x = 0; rgb_x < 4; rgb_x += 1) begin
                        rgb_data_up[rgb_y][rgb_x] <= 0;
                    end
                end
            end
            DECODE: begin
                rgb_data_up <= rgb_data_up_next;
            end
        endcase

        state <= state_next;
        done <= done_next;
    end
end


always_comb begin
    state_next = state;
    done_next = 0;
    rgb_data_up_next = rgb_data_up;

    case (state)
        IDLE: begin
            if (start) begin
                state_next = DECODE;
            end
        end
        DECODE: begin
            integer i;
            for (rgb_y = 0; rgb_y < 4; rgb_y += 1) begin
                for (rgb_x = 0; rgb_x < 4; rgb_x += 1) begin
                    i = (rgb_y * 4) + rgb_x;
                    if (!bitmap[i]) begin
                        rgb_data_up_next[rgb_y][rgb_x] = {bucket_0_avg_r, bucket_0_avg_g, bucket_0_avg_b};
                    end else begin
                        rgb_data_up_next[rgb_y][rgb_x] = {bucket_1_avg_r, bucket_1_avg_g, bucket_1_avg_b};
                    end
                end
            end
            state_next = IDLE;
            done_next = 1;
        end
    endcase
end

endmodule
