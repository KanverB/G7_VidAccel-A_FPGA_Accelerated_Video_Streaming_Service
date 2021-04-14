# G7_VidAccel-A_FPGA_Accelerated_Video_Streaming_Service

## Description

We built a video streaming platform, essentially something similar to Netflix but much smaller in scope. For our project, we used FPGAs to accelerate the encoding and decoding part of video streaming since this takes a significant portion of time on a CPU. 

Our project uses a Desktop TCP Server to take in a input video and split it into many RGB 888 frames. Those frames are sent to a FPGA TCP Server which encodes the frames using a color cell compression hardware and stores them on a SD Card. Then a FPGA TCP Client would request the encoded frames and decode them using hardware. A Desktop TCP Client would then request the decoded frames the FPGA TCP Client and display them on a GUI for viewing.
