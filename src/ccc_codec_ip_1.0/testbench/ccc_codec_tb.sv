`define BITS_PER_PIXEL (3 * 8)
`define BITS_PER_BLOCK (16 + 48)

module ccc_codec_tb();


// Change these to run testbench

// localparam WIDTH = 4;
// localparam HEIGHT = 4;
// string IN_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/smile_4x4.ppm";
// string OUT_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/out_smile_4x4.ppm";
// localparam COMMENT_AFTER_PX = 0;
// localparam P3_IN = 0;    // Decimal data input

// localparam WIDTH = 16;
// localparam HEIGHT = 16;
// string IN_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/smile_16x16.ppm";
// string OUT_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/out_smile_16x16.ppm";
// localparam COMMENT_AFTER_PX = 0;
// localparam P3_IN = 0;    // Decimal data input

// localparam WIDTH = 80;
// localparam HEIGHT = 60;
// string IN_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/rgb0_80x60.ppm";
// string OUT_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/out_rgb0_80x60.ppm";
// localparam COMMENT_AFTER_PX = 1;
// localparam P3_IN = 1;    // Decimal data input

localparam WIDTH = 160;
localparam HEIGHT = 120;
string IN_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/rgb0_160x120.ppm";
string OUT_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/out_rgb0_160x120.ppm";
localparam COMMENT_AFTER_PX = 1;
localparam P3_IN = 1;    // Decimal data input


logic clk;
logic rst;

logic [HEIGHT*WIDTH*`BITS_PER_PIXEL - 1 : 0]            rgb_data_in;
logic [`BITS_PER_PIXEL - 1 : 0]                         rgb_data_in_up [0:HEIGHT-1] [0:WIDTH-1];

logic [HEIGHT/4*WIDTH/4*`BITS_PER_BLOCK - 1 : 0]    ccc_data;
logic [`BITS_PER_BLOCK - 1 : 0]                     ccc_data_up [0:HEIGHT/4-1] [0:WIDTH/4-1];

logic [HEIGHT*WIDTH*`BITS_PER_PIXEL - 1 : 0]            rgb_data_out;
logic [`BITS_PER_PIXEL - 1 : 0]                         rgb_data_out_up [0:HEIGHT-1] [0:WIDTH-1];

logic encoder_start;
logic decoder_start;

logic encoder_done;
logic decoder_done;

ccc_encoder # (
    .WIDTH(WIDTH),
    .HEIGHT(HEIGHT)
) encoder_dut (
    .clk(clk),
    .rst(rst),

    .rgb_data(rgb_data_in),
    .ccc_data(ccc_data),

    .start(encoder_start),
    .done(encoder_done)
);

ccc_decoder # (
    .WIDTH(WIDTH),
    .HEIGHT(HEIGHT)
) decoder_dut (
    .clk(clk),
    .rst(rst),

    .ccc_data(ccc_data),
    .rgb_data(rgb_data_out),

    .start(decoder_start),
    .done(decoder_done)
);

generate
    genvar y, x;
    for (y = 0; y < HEIGHT; y += 1) begin
        for (x = 0; x < WIDTH; x += 1) begin
            assign rgb_data_in[(y*WIDTH+x)*`BITS_PER_PIXEL +: `BITS_PER_PIXEL] = rgb_data_in_up[y][x];
            assign rgb_data_out_up[y][x] = rgb_data_out[(y*WIDTH+x)*`BITS_PER_PIXEL +: `BITS_PER_PIXEL];
        end
    end
    for (y = 0; y < HEIGHT/4; y += 1) begin
        for (x = 0; x < WIDTH/4; x += 1) begin
            assign ccc_data_up[y][x] = ccc_data[(y*WIDTH/4+x)*`BITS_PER_BLOCK +: `BITS_PER_BLOCK];
        end
    end
endgenerate


initial begin
    clk <= 0;
end
always begin
    #1 clk <= ~clk;
end

initial begin
    #1000;
    $display("Testbench timeout - ending now");
    $finish;
end

integer     input_file;
reg[8*40:1] line_in = 0;
reg[8*2:1]  file_type;
integer     hsize_s;
integer     vsize_s;
integer     max_encoding;

integer n_Temp;

reg [`BITS_PER_PIXEL-1:0] pixel_in;
reg [8-1:0] r_in;
reg [8-1:0] g_in;
reg [8-1:0] b_in;
integer in_y;
integer in_x;

reg [8-1:0] dummy_byte;

initial begin
    $display("Starting testbench");
    
    read_file();

    rst <= 1;
    encoder_start <= 0;
    decoder_start <= 0;
    #10;
    @(posedge clk);
    rst <= 0;
    #10;

    $display("Starting encoder");
    @(posedge clk);
    encoder_start <= 1;
    @(posedge clk);
    encoder_start <= 0;
    @(encoder_done);
    $display("Encoder done");
    #10;

    $display("Starting decoder");
    @(posedge clk);
    decoder_start <= 1;
    @(posedge clk);
    decoder_start <= 0;
    @(decoder_done);
    $display("Decoder done");
    #10;

    write_file();
    #10;

    $display("Done testbench");
    $finish;
end

task read_file();
    $display("Reading in file...");

    // Reading in PPM from file adapted from
    // https://forums.xilinx.com/t5/Video-and-Audio/Video-Beginner-Series-3-RTL-simulation-with-input-from-an-image/td-p/853243

    // Reading in binary data adapted from
    // https://stackoverflow.com/questions/25024751/read-binary-file-data-in-verilog-into-2d-array

    input_file = $fopen(IN_FILE_NAME,"rb");

    // Read and check file type
    $fgets(line_in,input_file);
    $sscanf(line_in,"%s",file_type);
    $display("Note: File type : %s",file_type);

    // May need to read and discard a line here
    $display("COMMENT_AFTER_PX = %d", COMMENT_AFTER_PX);
    if (COMMENT_AFTER_PX == 1) begin
        $fgets(line_in,input_file);
        $display("Read and discarded line: %s",line_in);
    end
    $display("P3_IN = %d", P3_IN);

    // Get the picture size
    // The width and weight need to be on the same line
    // separated with a space character
    $fscanf(input_file, "%d %d",hsize_s,vsize_s);
    // Print the width in the console
    $display("Note: Width: %0d",hsize_s);
    // Print the height in the console
    $display("Note: Height: %0d",vsize_s);

    // Get max encoding value and print it in the console
    $fscanf(input_file, "%d", max_encoding);
    $display("Note: Max encoding: %0d",max_encoding);

    // Has an extra 0x0a (LF) byte before the image data
    n_Temp = $fread(dummy_byte, input_file);
    $display("Read and discarded byte: 0x%02h", dummy_byte);

    for (in_y = 0; in_y < HEIGHT; in_y = in_y + 1) begin
        for (in_x = 0; in_x < WIDTH; in_x = in_x + 1) begin
            if (P3_IN == 1) begin
                $fscanf(input_file, "%d %d %d", r_in, g_in, b_in);
                rgb_data_in_up[in_y][in_x] = {r_in, g_in, b_in};
                $display("Read decimal pixel (y=%0d, x=%0d): 0x%06h", in_y, in_x, rgb_data_in_up[in_y][in_x]);
            end
            else begin
                n_Temp = $fread(pixel_in, input_file);
                rgb_data_in_up[in_y][in_x] = pixel_in;
                $display("Read binary pixel (y=%0d, x=%0d): 0x%06h", in_y, in_x, rgb_data_in_up[in_y][in_x]);
            end
        end
    end

    // Close the file
    $fclose(input_file);
    $display("Done reading input file");
endtask;

integer output_file;

task write_file();
    $display("Writing out file...");

    // Writing out to PPM file adapted from
    // https://forums.xilinx.com/t5/Video-and-Audio/Video-Beginner-Series-5-Saving-simulation-outputs-to-an-image/td-p/860595

    output_file = $fopen(OUT_FILE_NAME,"w");
    $fwrite(output_file,"P3\n");
    $fwrite(output_file,"%0d %0d\n", WIDTH, HEIGHT);
    $fwrite(output_file,"%0d\n",2**8-1);

    for (in_y = 0; in_y < HEIGHT; in_y = in_y + 1) begin
        for (in_x = 0; in_x < WIDTH; in_x = in_x + 1) begin
            $fwrite(output_file,"%0d %0d %0d  ",
                int'(rgb_data_out_up[in_y][in_x][23:16]),
                int'(rgb_data_out_up[in_y][in_x][15:8]),
                int'(rgb_data_out_up[in_y][in_x][7:0]));
            $display("Wrote binary pixel (y=%0d, x=%0d): 0x%06h", in_y, in_x, rgb_data_out_up[in_y][in_x]);
        end
        $fwrite(output_file, "\n");
    end

    // Close the file
    $fclose(output_file);
    $display("Done writing output file");
endtask;

endmodule
