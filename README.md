# G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service

## Video Demo

Below is a video showcasing our project.

https://user-images.githubusercontent.com/47866804/114767729-00136d00-9d36-11eb-8a88-864875088e79.mp4

## Description

We built a video streaming platform, essentially something similar to Netflix but much smaller in scope. For our project, we used FPGAs to accelerate the encoding and decoding part of video streaming since this takes a significant portion of time on a CPU. 

Our project uses a Desktop TCP Server to take in an input video and split it into many RGB 888 frames. Those frames are sent to a FPGA TCP Server which encodes the frames using a color cell compression hardware and stores them on a SD Card. Then a FPGA TCP Client would request the encoded frames and decode them using hardware. A Desktop TCP Client would then request the decoded frames from the FPGA TCP Client and display them on a GUI for viewing.

## How to use

1. Change the host IP addr in frame_splitter.py to match your PC's IP. Modify the file path for where the source video is located in frame_splitter.py. Run frame_splitter.py. This step sets this PC as the Desktop TCP Server.
2. Create the Vivado project and generate the bitstream. Then open the Xilinx SDK and create a new project based on the lwIP Echo Server template. Replace the following files main.c, echo.c, xaxiemacif_physpeed.c (if using the Nexys Video Board), and lwipopts.h (or modify the BSP settings to match this file). Set the macro CLIENT to 0 in main.c and echo.c and change the DEST_IP4_ADDR to the addr used in the Desktop TCP Server. Also, change the mac_ethernet_address in main.c to match your board and the src IPV4_ADDR to match your board. Start the program. This step sets this FPGA as the FPGA TCP Server.
3. Then using another FPGA board follow the same steps as 2 but using the new board's IPV4 addr and MAC addr and setting the DEST_IP4_ADDR to the addr used by the FPGA TCP Server. Set the marco CLIENT to 1 in main.c and echo.c. This sets the FPGA as the FPGA TCP Client.
4. Change the host IP addr in frame_splitter.py to match the FPGA TCP Client's IP. Make sure to specify the directory that the ppm files should be written into in frame_viewer.py. This step sets this PC as the Desktop TCP Server. Then after the video has been transferred it should play on a GUI! The start and stop buttons can be clicked to start or stop the video. Debugging and logging messages are printed onto the terminal while frames are being sent from the FPGA TCP Client.

## Repository Structure

root

--docs/: Directory holding all our documentation.

----[Final Demo Presentation](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/docs/Final%20Demo%20Presentation.pdf): Our slides that were used in the final presentation of our project.

----[Final Report](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/docs/Final%20Report.pdf): A report providing an overview of our project, the outcome, our schedule, a description of the IP blocks, and tips and tricks. 

----[Video](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/docs/project_demo.mp4): A video demo of our project. 

--Example_Video/: Directory containing our example source video to send using our project.

----[IMG_3138.mp4](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/Example_Video/IMG_3138.mp4): The video we encoded and decoded. A very cute cat.

--LWIP_Modifcations/: Changes we made to LWIP driver files.

----[xaxiemacif_physpeed.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/LWIP_Modifcations/xaxiemacif_physpeed.c): Modifications done to the default LWIP xaxiemacif_physpeed.c file. Added support for Realtek Ethernet PHYs.

--PC_GUI_Server_Client/: The code for our Desktop TCP Server and Desktop TCP Client. Also contains a GUI.

----[frame_splitter.py](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/PC_GUI_Server_Client/frame_splitter.py): Code for Desktop TCP Server. Also, splits a video into RGB 888 frames.

----[frame_viewer.py](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/PC_GUI_Server_Client/frame_viewer.py): Code for Desktop TCP Client. Also, displays the decoded frames on a GUI and prints to the terminal debugging and logging messages from the FPGA TCP Client.

--src/:

----ccc_codec_ip_1.0/: Directory containing src code and testbenches for our encoder and decoder.

------hdl/: Directory containing our src verilog files.

--------[ccc_codec_ip_v1_0.v](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/hdl/ccc_codec_ip_v1_0.v): AXI Lite interface src code wrapper.

--------[ccc_codec_ip_v1_0_S00_AXI.v](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/hdl/ccc_codec_ip_v1_0_S00_AXI.v): AXI Lite interface src code.

--------[ccc_decoder.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/hdl/ccc_decoder.sv): Decoder src code.

--------[ccc_decoder_4x4.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/hdl/ccc_decoder_4x4.sv): Decoder src code that generates multiple decoders.

--------[ccc_encoder.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/hdl/ccc_encoder.sv): Encoder src code.

--------[ccc_encoder_4x4.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/hdl/ccc_encoder_4x4.sv): Encoder src code that generates multiple encoders.

------MicroBlaze_test/: Directory containing our MicroBlaze testing code.

--------[ccc_codec_test_\*.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/tree/main/src/ccc_codec_ip_1.0/MicroBlaze_test): 5 test files that each run a different test on the encoder and decoder.

------testbench/: Directory containing our testbench code.

--------[ccc_codec_axi_vip_tb.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/testbench/ccc_codec_axi_vip_tb.sv): AXI VIP testbench src code.

--------[ccc_codec_tb.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/testbench/ccc_codec_tb.sv): Src code for codec testbench.

--------[ece342_vga_bmp.sv](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/ccc_codec_ip_1.0/testbench/ece342_vga_bmp.sv): testbench to show output as a bitmap.

--------tb_frames\: Directory containing a variety of test images in .png and .ppm format.

----------[\*.ppm and \*.png images](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/tree/main/src/ccc_codec_ip_1.0/testbench/tb_frames): A variety of test images used in the testbench for the encoder and decoder.

----constraints/: Directory containing constraints for our boards SD Card pins.

------[SDpin.xdc](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/constraints/SDpin.xdc): Constraints file containing our SD Card pin mappings for the Nexys Video Board.

----MicroBlaze_Code/: Directory containing src code for FPGA TCP Server and FPGA TCP Client.

------[echo.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/MicroBlaze_Code/echo.c): The src code for our FPGA TCP Server/Client. Used to send the frames to anyone who connects. Messages or prints are logged using UART onto a terminal.

------[lwipopts.h.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/MicroBlaze_Code/lwipopts.h): The src code for the parameters we choose for the LWIP driver code. These parameters can be modified using the Modify BSP Settings button in the system.mss file using Xilinx SDK.

------[main.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/MicroBlaze_Code/main.c): The src code for our FPGA TCP Server/Client. Used to receive frames. Also encodes/decodes the frames and writes them to the SD Card. Messages or prints are logged using UART onto a terminal.

------[platform.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/MicroBlaze_Code/platform.c): The src code for adding interrupts for UART into MicroBlaze.

----SD_MYCODE_1.0/: Directory containing src code and test drivers for our sd_controller interface. MircoBlaze would communicate with the SD Card using this interface. 

------hdl/: Directory containing our src verilog files.

--------[SD_MYCODE_v1_0.v](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/SD_MYCODE_1.0/hdl/SD_MYCODE_v1_0.v): AXI Lite interface src code wrapper.

--------[SD_MYCODE_v1_0_S00_AXI.v](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/SD_MYCODE_1.0/hdl/SD_MYCODE_v1_0_S00_AXI.v): AXI Lite interface src code.

------MicroBlaze_test/: Directory containing our MicroBlaze testing code.

--------[sdcarddriver.c](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/SD_MYCODE_1.0/MicroBlaze_test/sdcarddriver.c): Test file that runs on MicroBlaze that writes and reads blocks to the SD Card.

----SD_SPI/: Directory containing src code of our modifications to the SD Card SPI interface we used from MIT. 

------hdl/: Directory containing our src verilog files.

--------[SD_SPI.v](https://github.com/KanverB/G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service/blob/main/src/SD_SPI/hdl/SD_SPI.v): Src code for the modifications we made to the SD Card SPI controller from MIT.

## Authors

Tianyi Zhang,
Kanver Bhandal,
Bruno Almeida 

## Acknowledgements

Professor Chow - For being an amazing professor who taught us many useful concepts that we used throughout our project. 

Daniel Rozhko - Our TA who provided us with lots of helpful feedback and helped answer any questions we had.

MIT - For their open source SD Card Controller SPI IP. The original SD Card SPI code can be found here: http://web.mit.edu/6.111/www/f2015/tools/sd_controller.v

Group 1 - Hyperspectral Image Compression and Decompression on FPGAs - For providing us with help on what modifications needed to be done to the SD Card Controller IP and for sending us the source of the IP they used for the SD Card.
