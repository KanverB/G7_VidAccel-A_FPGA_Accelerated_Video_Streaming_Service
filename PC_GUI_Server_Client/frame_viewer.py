import numpy as np
import cv2
import sys
import os
import socket
import serial
import _thread
import time

#change these for your setup!
HOST = "1.1.24.2"  # The remote server's hostname or IP address
PORT = 50000  # The port used by the remote server

#Frame size in bytes for 160x120 pixels
FRAME_SIZE = 57615

#global to stop/start video
stop_video = False

#global saying if buffer is empty
buffer_empty = True

#if stop = 1 then stop (1) is the current position
#if stop = 0 then start (0) is the the current position
def start_stop(stop):
    global stop_video 
    if (stop):
        stop_video = True
    else:
        stop_video = False

#client code
#it receives the RGB ppm files and writes them to the directory
def client():
    global buffer_empty
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        i = 0
        finished_transfer = False
        
        while (i != 137):
            recievedFrameBtyes = 0;
            frame = b''
            #read in one whole frame
            while (recievedFrameBtyes < FRAME_SIZE):
                #read in the sent packet
                data = s.recv(8192)
                # Check if read unsuccessful 
                if not data:
                    finished_transfer = True              
                    break
                frame += data
                recievedFrameBtyes += len(data)
                #print(data)
            
            #read in frame done now write it to a file
            with open('rgb'+str(i)+'_test.ppm','wb') as f:
                f.write(frame)
                
            if finished_transfer: 
                break
                
            i += 1
            
            #let server know we read one frame
            s.sendall(b"Received frame")
        
        buffer_empty = False
        s.shutdown(1)
        s.close()
        
#https://maker.pro/pic/tutorial/introduction-to-python-serial-ports
def perf_monitor():
    serialPort = serial.Serial(port = "COM5", baudrate=9600, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE)    
    serialString = "" # Used to hold data coming over UART
    while(1):
        # Wait until there is data waiting in the serial buffer
        if(serialPort.in_waiting > 0):
            # Read data out of the buffer until a carraige return / new line is found
            serialString = serialPort.readline()
            # Print the contents of the serial data
            print(serialString.decode('Ascii'))    
 
def main():
    _thread.start_new_thread(client,())
    #_thread.start_new_thread(perf_monitor,())
    i=0
    #switch to directory with .ppm files
    #is hardcoded should fix this later
    os.chdir('W:/PortableGit/ece532/')
    #create a window and add buttons to it for start and stop
    cv2.namedWindow('VidAccel',cv2.WINDOW_GUI_NORMAL | cv2.WINDOW_NORMAL | cv2.WINDOW_KEEPRATIO)
    #use the trackbar as a workaround for a button on Windows
    cv2.createTrackbar('Start/Stop','VidAccel',0,1,start_stop)
    #wait for buffer to fill up with some frames
    while (buffer_empty):
        cv2.waitKey(42)
        pass
    #read img using opencv
    img = cv2.imread('rgb'+str(i)+'_test.ppm')
    while(not(img is None)):
        #show image and wait for a frame update
        cv2.imshow('VidAccel',img)
        #we want to display 24 fps so each frame should come every 41.67ms 
        #have to round to int
        cv2.waitKey(42)
        #stop showing the next frame if user pressed stop
        if ((not stop_video) and (not buffer_empty)):
            i+=1
            img = cv2.imread('rgb'+str(i)+'_test.ppm')
    
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
