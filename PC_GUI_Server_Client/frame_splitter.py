import numpy as np
import cv2
import sys
import os
import socket
import _thread

#remember to update these for your computer
HOST = '1.1.30.1' # The IP address of the computer this script runs on
PORT = 50000 # The port used by this TCP server 

def open_new_client(connection, addr):
    print("Connected to client ", addr[0], " port ", addr[1])
    
    #start with frame 0
    i = 0

    # Loop until connection closed
    while True:
        # Send frame data to client
        #read in the frame
        if (i != 137):
            with open('rgb'+str(i)+'.ppm','rb') as f:
                data = f.read()
                #each frame is a fixed 921,615 bytes so for now ignore sending the frame size
                #find the size of the file being sent and send that first
                #this so the client knows how many bytes to read
                #size = os.path.getsize('rgb'+str(i)+'.ppm')
                #connection.sendall(size)
                #repsonse = connection.recv(1024)
                #print("Received", repr(repsonse))
                #if not response: break
                #send it
                connection.sendall(data)
                i += 1
        
        # Read response
        data = connection.recv(1024)

        # Check if read unsuccessful 
        if not data: break

        print("Received", repr(data))

    # Close the connection if break from loop
    connection.shutdown(1)
    connection.close()
    print("Connection to client ", addr[0], " port ", addr[1], " closed")

def listen():
    # Setup the socket
    connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connection.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind to an address and port to listen on
    connection.bind((HOST, PORT))
    connection.listen(10)
    print("Server opened on ",HOST," port ", PORT)

    # Loop forever, accepting all connections in new thread
    while True:
        new_conn, new_addr = connection.accept()
        _thread.start_new_thread(open_new_client,(new_conn, new_addr))
 
def main():
    #place directory path of movie and open it using opencv
    #is hardcoded should fix this later
    cap = cv2.VideoCapture('W:/PortableGit/ece532/IMG_3138.mp4')
    i=0
    #change directory to where you want to output files
    #is hardcoded should fix this later
    os.chdir('W:/PortableGit/ece532/') 
    #keep reading frames of a movie and outputting them as .ppm files
    #https://en.wikipedia.org/wiki/Netpbm (uncompressed raw rgb)
    #check if files have already been split
    if not (os.path.exists("rgb0.ppm")):
        while(cap.isOpened()):
            #reads a frame from the movie
            ret, frame = cap.read()
            if ret == False:
                break
            #resize frame to 160x120
            frame_small = cv2.resize(frame,(160,120),fx=0,fy=0,interpolation = cv2.INTER_CUBIC)
            #write the frame as rgb_x.ppm
            cv2.imwrite('rgb'+str(i)+'.ppm',frame_small)
            i+=1
    
    cap.release()
    cv2.destroyAllWindows()
    
    #after splitting frames let's start up the server!
    try:
        listen()
    except KeyboardInterrupt:
        pass  

if __name__ == "__main__":
    main()