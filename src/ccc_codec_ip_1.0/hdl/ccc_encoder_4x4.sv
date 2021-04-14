`define BITS_PER_PIXEL (3 * 8)
`define BITS_PER_BLOCK (16 + 48)

module ccc_encoder_4x4 (
    input clk,
    (* mark_debug = "true" *) input rst,

    (* mark_debug = "true" *) input [4*4*`BITS_PER_PIXEL - 1 : 0] rgb_data,
    (* mark_debug = "true" *) output reg [`BITS_PER_BLOCK - 1 : 0] ccc_data,

    (* mark_debug = "true" *) input start,
    (* mark_debug = "true" *) output reg done
);

// These signals are for easier viewing of waveforms
(* mark_debug = "true" *) wire [`BITS_PER_PIXEL - 1 : 0] rgb_data_up [0:3] [0:3];
(* mark_debug = "true" *) reg [`BITS_PER_BLOCK - 1 : 0] ccc_data_up;
generate
    genvar y, x;
    for (y = 0; y < 4; y += 1) begin
        for (x = 0; x < 4; x += 1) begin
            assign rgb_data_up[y][x] = rgb_data[(y*4+x)*`BITS_PER_PIXEL +: `BITS_PER_PIXEL];
        end
    end
endgenerate
assign ccc_data_up = ccc_data;

typedef enum {
    IDLE,
    CALC_LUM,
    SORT,
    CALC_BITMAP,
    CALC_AVG,
    SET_CCC
} STATE;

(* mark_debug = "true" *) STATE state;
(* mark_debug = "true" *) STATE state_next;

(* mark_debug = "true" *) reg done_next;

(* mark_debug = "true" *) reg [3 - 1 : 0] calc_lum_counter;
(* mark_debug = "true" *) reg [16 - 1 : 0] rgb_lum_calc [16];   // temporary for accumulation/calculation
(* mark_debug = "true" *) reg [8 - 1 : 0] rgb_lum [16];

(* mark_debug = "true" *) reg [8 - 1 : 0] sorted_lum_values [16];
(* mark_debug = "true" *) reg [8 - 1 : 0] sorted_lum_values_next [16];
(* mark_debug = "true" *) reg [4 - 1 : 0] sorted_lum_indices [16];
(* mark_debug = "true" *) reg [4 - 1 : 0] sorted_lum_indices_next [16];

// If sorted_compare[i] is 1, the next value to be added to the array is less
// than or equal to the value at sorted_lum_values[i]
(* mark_debug = "true" *) reg sorted_compare [16];
(* mark_debug = "true" *) reg [4 - 1 : 0] sorting_index;
// If move_down[i] is 1, the value at sorted_lum_values[i] will be evicted and
// moved to sorted_lum_values[i+1]
(* mark_debug = "true" *) reg move_down [16];

(* mark_debug = "true" *) reg bitmap [16];
(* mark_debug = "true" *) reg [16 - 1 : 0] bitmap_p;
generate
    genvar gen_i;
    for (gen_i = 0; gen_i < 16; gen_i += 1) begin
        assign bitmap_p[gen_i] = bitmap[gen_i];
    end
endgenerate

(* mark_debug = "true" *) reg [4 - 1 : 0] calc_avg_counter;
// Make these 11 bits just in case, since we >> 3 later
(* mark_debug = "true" *) reg [11 - 1 : 0] bucket_0_avg_r;
(* mark_debug = "true" *) reg [11 - 1 : 0] bucket_0_avg_g;
(* mark_debug = "true" *) reg [11 - 1 : 0] bucket_0_avg_b;
(* mark_debug = "true" *) reg [11 - 1 : 0] bucket_1_avg_r;
(* mark_debug = "true" *) reg [11 - 1 : 0] bucket_1_avg_g;
(* mark_debug = "true" *) reg [11 - 1 : 0] bucket_1_avg_b;


always_ff @(posedge clk) begin
    integer i;

    if (rst) begin
        state <= IDLE;
        done <= 0;
    end
    else begin
        case (state)
            IDLE: begin
                // Make sure to reset these values before the encoder starts processing a new image
                ccc_data <= 0;
                sorting_index <= 0;

                calc_lum_counter <= 0;
                calc_avg_counter <= 0;

                for (i = 0; i < 16; i += 1) begin
                    rgb_lum_calc[i] <= 0;
                    rgb_lum[i] <= 0;
                    sorted_lum_values[i] <= 8'hFF;
                    sorted_lum_indices[i] <= 0;
                    bitmap[i] <= 0;
                end

                bucket_0_avg_r <= 0;
                bucket_0_avg_g <= 0;
                bucket_0_avg_b <= 0;
                bucket_1_avg_r <= 0;
                bucket_1_avg_g <= 0;
                bucket_1_avg_b <= 0;
            end
            CALC_LUM: begin
                for (i = 0; i < 16; i += 1) begin
                    // This shouldn't be able to overflow
                    // Make sure multiplication and addition are 16-bit math
                    if (calc_lum_counter == 0) begin
                        // red
                        rgb_lum_calc[i] <= rgb_lum_calc[i] + (16'd77  * {8'd0, rgb_data[i*`BITS_PER_PIXEL+16 +: 8]});
                    end
                    if (calc_lum_counter == 1) begin
                        // green
                        rgb_lum_calc[i] <= rgb_lum_calc[i] + (16'd151 * {8'd0, rgb_data[i*`BITS_PER_PIXEL+ 8 +: 8]});
                    end
                    if (calc_lum_counter == 2) begin
                        // blue
                        rgb_lum_calc[i] <= rgb_lum_calc[i] + (16'd28  * {8'd0, rgb_data[i*`BITS_PER_PIXEL+ 0 +: 8]});
                    end
                    if (calc_lum_counter == 3) begin
                        rgb_lum[i] <= rgb_lum_calc[i] >> 8;
                    end
                end

                calc_lum_counter <= calc_lum_counter + 1;
            end
            SORT: begin
                sorted_lum_values <= sorted_lum_values_next;
                sorted_lum_indices <= sorted_lum_indices_next;
                sorting_index <= sorting_index + 1;
            end
            CALC_BITMAP: begin
                for (i = 0; i < 8; i += 1) begin
                    bitmap[sorted_lum_indices[i]] <= 0;
                end
                for (i = 8; i < 16; i += 1) begin
                    bitmap[sorted_lum_indices[i]] <= 1;
                end
            end
            CALC_AVG: begin
                // This declaration needs to go before any logic
                integer i_pix;

                if (calc_avg_counter < 8) begin
                    i_pix = sorted_lum_indices[calc_avg_counter];
                    bucket_0_avg_r <= bucket_0_avg_r + {3'd0, rgb_data[i_pix*`BITS_PER_PIXEL+16 +: 8]};
                    bucket_0_avg_g <= bucket_0_avg_g + {3'd0, rgb_data[i_pix*`BITS_PER_PIXEL+ 8 +: 8]};
                    bucket_0_avg_b <= bucket_0_avg_b + {3'd0, rgb_data[i_pix*`BITS_PER_PIXEL+ 0 +: 8]};

                    i_pix = sorted_lum_indices[8 + calc_avg_counter];
                    bucket_1_avg_r <= bucket_1_avg_r + {3'd0, rgb_data[i_pix*`BITS_PER_PIXEL+16 +: 8]};
                    bucket_1_avg_g <= bucket_1_avg_g + {3'd0, rgb_data[i_pix*`BITS_PER_PIXEL+ 8 +: 8]};
                    bucket_1_avg_b <= bucket_1_avg_b + {3'd0, rgb_data[i_pix*`BITS_PER_PIXEL+ 0 +: 8]};
                end

                else begin
                    bucket_0_avg_r <= bucket_0_avg_r >> 3;
                    bucket_0_avg_g <= bucket_0_avg_g >> 3;
                    bucket_0_avg_b <= bucket_0_avg_b >> 3;

                    bucket_1_avg_r <= bucket_1_avg_r >> 3;
                    bucket_1_avg_g <= bucket_1_avg_g >> 3;
                    bucket_1_avg_b <= bucket_1_avg_b >> 3;
                end

                calc_avg_counter <= calc_avg_counter + 1;
            end
            SET_CCC: begin
                ccc_data <= {
                    bitmap_p,       // 16 bits
                    bucket_0_avg_r[7:0], // 8 bits
                    bucket_0_avg_g[7:0], // 8 bits
                    bucket_0_avg_b[7:0], // 8 bits
                    bucket_1_avg_r[7:0], // 8 bits
                    bucket_1_avg_g[7:0], // 8 bits
                    bucket_1_avg_b[7:0]  // 8 bits
                };
            end
        endcase

        state <= state_next;
        done <= done_next;
    end
end


always_comb begin
    integer i;

    state_next = state;
    done_next = 0;

    sorted_lum_values_next = sorted_lum_values;
    sorted_lum_indices_next = sorted_lum_indices;

    for (i = 0; i < 16; i += 1) begin
        move_down[i] = 0;
        sorted_compare[i] = 0;
    end

    case (state)
        IDLE: begin
            if (start) begin
                state_next = CALC_LUM;
            end
        end
        CALC_LUM: begin
            if (calc_lum_counter == 3) begin
                state_next = SORT;
            end
        end
        SORT: begin
            // Compare next value to add with all existing values in array
            for (i = 0; i < 16; i += 1) begin
                sorted_compare[i] = (rgb_lum[sorting_index] <= sorted_lum_values[i]);
            end

            // Figure out which values should be evicted and moved down
            move_down[0] = sorted_compare[0];
            for (i = 1; i < 16; i += 1) begin
                move_down[i] = move_down[i - 1] || sorted_compare[i];
            end

            // Assign next values for sorted array
            for (i = 1; i < 16; i += 1) begin
                if (move_down[i - 1]) begin
                    sorted_lum_values_next[i] = sorted_lum_values[i - 1];
                    sorted_lum_indices_next[i] = sorted_lum_indices[i - 1];
                end
            end
            for (i = 0; i < 16; i += 1) begin
                if (sorted_compare[i] && (i == 0 || !move_down[i - 1])) begin
                    sorted_lum_values_next[i] = rgb_lum[sorting_index];
                    sorted_lum_indices_next[i] = sorting_index;
                end
            end
            
            // Finish sorting if done
            if (sorting_index == 15) begin
                state_next = CALC_BITMAP;
            end
        end
        CALC_BITMAP: begin
            state_next = CALC_AVG;
        end
        CALC_AVG: begin
            if (calc_avg_counter == 8) begin
                state_next = SET_CCC;
            end
        end
        SET_CCC: begin
            state_next = IDLE;
            done_next = 1;
        end
    endcase
end

endmodule
