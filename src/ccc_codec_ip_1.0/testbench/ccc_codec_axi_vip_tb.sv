`timescale 1ns / 1ps

`define BITS_PER_PIXEL (3 * 8)
`define BITS_PER_BLOCK (16 + 48)

import axi_vip_pkg::*;
import design_1_axi_vip_0_0_pkg::*;

//test module to drive the AXI VIP
module axi_lite_stimulus();
  /*************************************************************************************************
  * <component_name>_mst_t for master agent
  * <component_name> can be easily found in vivado bd design: click on the instance, 
  * Then click CONFIG under Properties window and Component_Name will be shown
  * More details please refer PG267 section about "Useful Coding Guidelines and Examples"
  * for more details.
  *************************************************************************************************/
  design_1_axi_vip_0_0_mst_t                               agent;

  /*************************************************************************************************
  * Declare variables which will be used in API and parital randomization for transaction generation
  * and data read back from driver.
  *************************************************************************************************/
  axi_transaction                                          wr_trans;            // Write transaction
  axi_transaction                                          rd_trans;            // Read transaction
  xil_axi_uint                                             mtestWID;            // Write ID  
  xil_axi_ulong                                            mtestWADDR;          // Write ADDR  
  xil_axi_len_t                                            mtestWBurstLength;   // Write Burst Length   
  xil_axi_size_t                                           mtestWDataSize;      // Write SIZE  
  xil_axi_burst_t                                          mtestWBurstType;     // Write Burst Type  
  xil_axi_lock_t                                           mtestWLock;          // Write Lock Type  
  xil_axi_cache_t                                          mtestWCache;          // Write Cache Type  
  xil_axi_prot_t                                           mtestWProt;          // Write Prot Type  
  xil_axi_region_t                                         mtestWRegion;        // Write Region Type  
  xil_axi_qos_t                                            mtestWQos;           // Write Qos Type  
  xil_axi_uint                                             mtestRID;            // Read ID  
  xil_axi_ulong                                            mtestRADDR;          // Read ADDR  
  xil_axi_len_t                                            mtestRBurstLength;   // Read Burst Length   
  xil_axi_size_t                                           mtestRDataSize;      // Read SIZE  
  xil_axi_burst_t                                          mtestRBurstType;     // Read Burst Type  
  xil_axi_lock_t                                           mtestRLock;          // Read Lock Type  
  xil_axi_cache_t                                          mtestRCache;         // Read Cache Type  
  xil_axi_prot_t                                           mtestRProt;          // Read Prot Type  
  xil_axi_region_t                                         mtestRRegion;        // Read Region Type  
  xil_axi_qos_t                                            mtestRQos;           // Read Qos Type  

  xil_axi_data_beat                                        Rdatabeat[];       // Read data beats

  //Constants

  localparam ITERATIONS = 2;

  localparam WIDTH = 4;
  localparam HEIGHT = 4;
  string IN_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/smile_4x4.ppm";
  string OUT_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/out_smile_4x4.ppm";
  localparam COMMENT_AFTER_PX = 0;
  localparam P3_IN = 0;    // Decimal data input

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

  // localparam WIDTH = 160;
  // localparam HEIGHT = 120;
  // string IN_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/rgb0_160x120.ppm";
  // string OUT_FILE_NAME = "W:/ece532/vidaccel/src/tb_frames/out_rgb0_160x120.ppm";
  // localparam COMMENT_AFTER_PX = 1;
  // localparam P3_IN = 1;    // Decimal data input

  // Chunk is defined as WIDTH * 4 pixels, i.e. 4 rows of image
  localparam BITS_PER_CHUNK = WIDTH*4*`BITS_PER_PIXEL;

  //AXI register offsets

  // Addresses
	localparam ENC_START_ADDR 		  = 'h000;
	localparam ENC_DONE_ADDR 	 	    = 'h004;
	localparam ENC_RGB_ADDR_BASE  	= 'h008;
	localparam ENC_RGB_ADDR_SIZE  	= 'h040;
	localparam ENC_CCC_ADDR_BASE  	= 'h048;
	localparam ENC_CCC_ADDR_SIZE  	= 'h008;

	localparam DEC_START_ADDR 		  = 'h050;
	localparam DEC_DONE_ADDR  		  = 'h054;
	localparam DEC_CCC_ADDR_BASE  	= 'h058;
	localparam DEC_CCC_ADDR_SIZE  	= 'h008;
	localparam DEC_RGB_ADDR_BASE  	= 'h060;
	localparam DEC_RGB_ADDR_SIZE  	= 'h040;


  logic [`BITS_PER_PIXEL - 1 : 0] rgb_data_in_up [0:HEIGHT-1] [0:WIDTH-1];
  logic [`BITS_PER_PIXEL - 1 : 0] rgb_data_out_up [0:HEIGHT-1] [0:WIDTH-1];


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

  integer pix_y;
  integer pix_x;


  initial begin
    /***********************************************************************************************
    * Before agent is newed, user has to run simulation with an empty testbench to find the hierarchy
    * path of the AXI VIP's instance.Message like
    * "Xilinx AXI VIP Found at Path: my_ip_exdes_tb.DUT.ex_design.axi_vip_mst.inst" will be printed 
    * out. Pass this path to the new function. 
    ***********************************************************************************************/
    agent = new("master vip agent",DUT.design_1_i.axi_vip_0.inst.IF);
    agent.start_master();               // agent start to run

    //Put test vectors here

    $display("Starting testbench");
  
    for (integer iter = 0; iter < ITERATIONS; iter += 1) begin
      read_file();
      #10;

      
      // For each block of 4 rows, 4 columns
      for (integer rows_block = 0; rows_block < HEIGHT / 4; rows_block += 1) begin
        for (integer cols_block = 0; cols_block < WIDTH / 4; cols_block += 1) begin
          $display("Starting rows block %d, columns block %d", rows_block, cols_block);


          // Write input data to AXI registers
          $display("Writing input RGB data to registers...");
          
          for (integer y_in_block = 0; y_in_block < 4; y_in_block += 1) begin
            for (integer x_in_block = 0; x_in_block < 4; x_in_block += 1) begin
              integer i_pixel_in_block;
              integer addr;

              pix_y = (rows_block * 4) + y_in_block;
              pix_x = (cols_block * 4) + x_in_block;

              i_pixel_in_block = (y_in_block * 4) + x_in_block;
              addr = ENC_RGB_ADDR_BASE + (i_pixel_in_block * 4);

              // $display("Writing register at addr 0x%03x: 0x%08x", addr, rgb_data_in_up[pix_y][pix_x]);
              writeRegister(addr, rgb_data_in_up[pix_y][pix_x]);
            end
          end
          agent.wait_drivers_idle();


          $display("Starting encoder");
          writeRegister(ENC_START_ADDR, 1);
          agent.wait_drivers_idle();
          while (1) begin
            // $display("Waiting for encoder done...");
            readRegister(ENC_DONE_ADDR, Rdatabeat);
            // $display("Register value: 0x%08x", Rdatabeat[0]);
            if (Rdatabeat[0] == 1) begin
              break;
            end
          end
          agent.wait_drivers_idle();
          $display("Encoder done");


          // Copy contents of encoder CCC (out) buffer to decoder CCC (in) buffer
          for (integer addr_off = 0; addr_off < ENC_CCC_ADDR_SIZE; addr_off += 4) begin
            readRegister(ENC_CCC_ADDR_BASE + addr_off, Rdatabeat);
            agent.wait_drivers_idle();
            writeRegister(DEC_CCC_ADDR_BASE + addr_off, Rdatabeat[0]);
          end
          agent.wait_drivers_idle();


          $display("Starting decoder");
          writeRegister(DEC_START_ADDR, 1);
          agent.wait_drivers_idle();
          while (1) begin
            // $display("Waiting for decoder done...");
            readRegister(DEC_DONE_ADDR, Rdatabeat);
            // $display("Register value: 0x%08x", Rdatabeat[0]);
            if (Rdatabeat[0] == 1) begin
              break;
            end
          end
          agent.wait_drivers_idle();
          $display("Decoder done");


          // Read AXI registers, save to output data
          $display("Reading output RGB data from registers...");
          for (integer y_in_block = 0; y_in_block < 4; y_in_block += 1) begin
            for (integer x_in_block = 0; x_in_block < 4; x_in_block += 1) begin
              integer i_pixel_in_block;
              integer addr;

              pix_y = (rows_block * 4) + y_in_block;
              pix_x = (cols_block * 4) + x_in_block;

              i_pixel_in_block = (y_in_block * 4) + x_in_block;
              addr = DEC_RGB_ADDR_BASE + (i_pixel_in_block * 4);
              
              readRegister(addr, Rdatabeat);
              // $display("Reading register at addr 0x%03x: 0x%08x", addr, Rdatabeat[0]);
              rgb_data_out_up[pix_y][pix_x] = Rdatabeat[0];
            end
          end
          agent.wait_drivers_idle();
        end
      end

      #10;
      write_file();
      #10;
      agent.wait_drivers_idle();
    end

    $display("TB DONE");
    $finish;
 end


task read_file();
    $display("Reading input file: %s", IN_FILE_NAME);

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
    $display("Writing output file: %s", OUT_FILE_NAME);

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


task writeRegister( input xil_axi_ulong              addr =0,
                    input bit [31:0]              data =0
                );

    single_write_transaction_api("single write with api",
                                 .addr(addr),
                                 .size(xil_axi_size_t'(4)),
                                 .data(data)
                                 );
endtask : writeRegister

  /************************************************************************************************
  *  task single_write_transaction_api is to create a single write transaction, fill in transaction 
  *  by using APIs and send it to write driver.
  *   1. declare write transction
  *   2. Create the write transaction
  *   3. set addr, burst,ID,length,size by calling set_write_cmd(addr, burst,ID,length,size), 
  *   4. set prot.lock, cache,region and qos
  *   5. set beats
  *   6. set AWUSER if AWUSER_WIDH is bigger than 0
  *   7. set WUSER if WUSR_WIDTH is bigger than 0
  *************************************************************************************************/

  task automatic single_write_transaction_api ( 
                                input string                     name ="single_write",
                                input xil_axi_uint               id =0, 
                                input xil_axi_ulong              addr =0,
                                input xil_axi_len_t              len =0, 
                                input xil_axi_size_t             size =xil_axi_size_t'(xil_clog2((32)/8)),
                                input xil_axi_burst_t            burst =XIL_AXI_BURST_TYPE_INCR,
                                input xil_axi_lock_t             lock = XIL_AXI_ALOCK_NOLOCK,
                                input xil_axi_cache_t            cache =3,
                                input xil_axi_prot_t             prot =0,
                                input xil_axi_region_t           region =0,
                                input xil_axi_qos_t              qos =0,
                                input xil_axi_data_beat [255:0]  wuser =0, 
                                input xil_axi_data_beat          awuser =0,
                                input bit [32767:0]              data =0
                                                );
    axi_transaction                               wr_trans;
    wr_trans = agent.wr_driver.create_transaction(name);
    wr_trans.set_write_cmd(addr,burst,id,len,size);
    wr_trans.set_prot(prot);
    wr_trans.set_lock(lock);
    wr_trans.set_cache(cache);
    wr_trans.set_region(region);
    wr_trans.set_qos(qos);
    wr_trans.set_awuser(awuser);
    wr_trans.set_data_block(data);
    agent.wr_driver.send(wr_trans);   
  endtask  : single_write_transaction_api
  
  //task automatic readRegister (  input xil_axi_ulong addr =0 );
                                                
    //single_read_transaction_api(.addr(addr));

  //endtask  : readRegister
  task automatic readRegister ( 
                                    input xil_axi_ulong              addr =0,
                                    output xil_axi_data_beat Rdatabeat[]
                                                );
    axi_transaction                               rd_trans;
    xil_axi_uint               id =0; 
    xil_axi_len_t              len =0; 
    xil_axi_size_t             size =xil_axi_size_t'(xil_clog2((32)/8));
    xil_axi_burst_t            burst =XIL_AXI_BURST_TYPE_INCR;
    xil_axi_lock_t             lock =XIL_AXI_ALOCK_NOLOCK ;
    xil_axi_cache_t            cache =3;
    xil_axi_prot_t             prot =0;
    xil_axi_region_t           region =0;
    xil_axi_qos_t              qos =0;
    xil_axi_data_beat          aruser =0;
    rd_trans = agent.rd_driver.create_transaction("single-read");
    rd_trans.set_read_cmd(addr,burst,id,len,size);
    rd_trans.set_prot(prot);
    rd_trans.set_lock(lock);
    rd_trans.set_cache(cache);
    rd_trans.set_region(region);
    rd_trans.set_qos(qos);
    rd_trans.set_aruser(aruser);
    get_rd_data_beat_back(rd_trans,Rdatabeat);
  endtask  : readRegister
  
  /************************************************************************************************
  * Task send_wait_rd is a task which set_driver_return_item_policy of the read transaction, 
  * send the transaction to the driver and wait till it is done
  *************************************************************************************************/
  task send_wait_rd(inout axi_transaction rd_trans);
    rd_trans.set_driver_return_item_policy(XIL_AXI_PAYLOAD_RETURN);
    agent.rd_driver.send(rd_trans);
    agent.rd_driver.wait_rsp(rd_trans);
  endtask

  /************************************************************************************************
  * Task get_rd_data_beat_back is to get read data back from read driver with
  *  data beat format.
  *************************************************************************************************/
  task get_rd_data_beat_back(inout axi_transaction rd_trans, 
                                 output xil_axi_data_beat Rdatabeat[]
                            );  
    send_wait_rd(rd_trans);
    Rdatabeat = new[rd_trans.get_len()+1];
    for( xil_axi_uint beat=0; beat<rd_trans.get_len()+1; beat++) begin
      Rdatabeat[beat] = rd_trans.get_data_beat(beat);
   //   $display("Read data from Driver: beat index %d, Data beat %h ", beat, Rdatabeat[beat]);
    end  
  endtask


  /************************************************************************************************
  *  task single_read_transaction_api is to create a single read transaction, fill in command with user
  *  inputs and send it to read driver.
  *   1. declare read transction
  *   2. Create the read transaction
  *   3. set addr, burst,ID,length,size by calling set_read_cmd(addr, burst,ID,length,size), 
  *   4. set prot.lock, cache,region and qos
  *   5. set ARUSER if ARUSER_WIDH is bigger than 0
  *************************************************************************************************/
  task automatic single_read_transaction_api ( 
                                    input string                     name ="single_read",
                                    input xil_axi_uint               id =0, 
                                    input xil_axi_ulong              addr =0,
                                    input xil_axi_len_t              len =0, 
                                    input xil_axi_size_t             size =xil_axi_size_t'(xil_clog2((32)/8)),
                                    input xil_axi_burst_t            burst =XIL_AXI_BURST_TYPE_INCR,
                                    input xil_axi_lock_t             lock =XIL_AXI_ALOCK_NOLOCK ,
                                    input xil_axi_cache_t            cache =3,
                                    input xil_axi_prot_t             prot =0,
                                    input xil_axi_region_t           region =0,
                                    input xil_axi_qos_t              qos =0,
                                    input xil_axi_data_beat          aruser =0
                                                );
    axi_transaction                               rd_trans;
    rd_trans = agent.rd_driver.create_transaction(name);
    rd_trans.set_read_cmd(addr,burst,id,len,size);
    rd_trans.set_prot(prot);
    rd_trans.set_lock(lock);
    rd_trans.set_cache(cache);
    rd_trans.set_region(region);
    rd_trans.set_qos(qos);
    rd_trans.set_aruser(aruser);
    agent.rd_driver.send(rd_trans);   
  endtask  : single_read_transaction_api
endmodule 

//top level test bench
module tb(

    );

//signal declarations
logic aclk;
logic aresetn;

//instantiate instance of axi_lite_stimulus into the tb
axi_lite_stimulus mst();

//clock generator (100MHz)
initial
begin
	aclk = 0;
	forever
		#5ns aclk = ~aclk;
end

//start the testbench by resetting the system for 5 cycles
initial
begin
	aresetn = 0;
	repeat(5) @(posedge aclk);
	forever
		@(posedge aclk)aresetn = 1;
end

//instance of the block design (design 1)
design_1_wrapper DUT (.aclk(aclk), .aresetn(aresetn));

endmodule



