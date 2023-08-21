import sys
import socket
import select
import time
import queue
import io
#import os

command = ""
value = 0

##get input file socket info


#f = open(readfile, "r")
x = 0

#create a TCP/IP socket
server=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#set the socket to non-blocking mode
server.setblocking(0)
#print("yeahhh")

ip = str(sys.argv[1])
port = int(sys.argv[2])
buffer = int(sys.argv[3])
payload = int(sys.argv[4])
'''
print("ip_address:"+ip)
print("port num:%d" % port)
print("buffer:%d" % buffer)
print("payload:%d" % payload)
'''
server.bind((ip,port))

#havent used
seqnum = []
#havent used
acknum = []


wait_client_data = 0




#first packet

#reciever
#SYN|FIN|DAT|ACK|RST, 
#check if SYN, send synack
uncheck_data = []

potential_reader=[server]
potential_writer=[]
potential_exp=[]


data_section = 0


packet = ""
conn_tp = "SYN" 
fin_tp = "FIN"
win1 = 0
win2 = 0



recv_socket = []

file_fin = 0
send_data = 1
send_ack =""#the last ack info maybe
send_ack_n = 0#after reciever ack, add the number of sending ack
last_recv_ack = ""
recv_ack = []#the next sending ack info
recv_data = 0#after sending data, the data number should recv in future
resend_data = 0
second_data = 0
write = 0

finish_connection = 0
loss = 0
timeout = 0
pre_recv_ack = ""


length = 0 
finish = 0
msgls = ["ACK"]
sending_ack = 0



#################
connection_status = 0
mode = {}

mode[server] = "reader"
state = {}

inputs = []

seqno = {}#next seqno
ackno={}
client_win = {}
request_messages = {}
response_messages = {}

request_content = {}
server_win = {}
file = {}

while True:
    readable, writable, exceptional = select.select(potential_reader,potential_writer, potential_exp, 1)
    #print("reader:%d" % len(potential_reader))
    #print("writer:%d" % len(potential_writer))
 
    if server in readable:
        print("in readable")
        print("reader: %d" % len(potential_reader))
        ######################
        #make connection with client
        ######################

        try:

            m, clientsocket = server.recvfrom(buffer)
            mode[server] = "writer"
            #print("recieve client:" + clientsocket[0] +" "+ str(clientsocket[1]))

            m = m.decode()
            ml = m.split("\r\n")
            tp = ml[0]


            if clientsocket not in inputs:
                state[clientsocket] = "connect"

            if (state[clientsocket] == "connect"):

                if (tp == "SYN"):
                    print("%s: Recieve; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), int(ml[3].split(":")[1]), int(ml[4].split(":")[1])))



                    inputs.append(clientsocket)
                    request_messages[clientsocket] = queue.Queue()
                    response_messages[clientsocket] = queue.Queue()
                    server_win[clientsocket] = buffer
                    ackno[clientsocket] = 0



                    mode[clientsocket] = "writer"
                    request_messages[clientsocket].put(m)
                    potential_writer.append(server)


                elif (tp == "ACK"):
                    print("%s: Recieve; ACK; Sequence:%d; Length:%d; Acknowledgement: %d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), int(ml[3].split(":")[1]), int(ml[4].split(":")[1])))

                    seqno[clientsocket] = queue.Queue()

                    seqno[clientsocket].put(int(ml[3].split(":")[1]))
                    client_win[clientsocket] = int(ml[4].split(":")[1])
                    state[clientsocket] = "data"
                    mode[clientsocket] = "reader"


            ########################
            #recieve data from client
            #######################
            elif (state[clientsocket] == "data"):
                if (tp == "DAT"):
                    print("recive data from client!")
                    print("%s: Recieve; DAT; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), ackno[clientsocket], int(ml[4].split(":")[1])))
                    #it may have multiple requests here
                    seqno[clientsocket] = int(ml[3].split(":")[1])
                    ackno[clientsocket] += int(ml[2].split(":")[1])
                    client_win[clientsocket] = int(ml[4].split(":")[1])

                    request_content[clientsocket] = ml[6:-2]

                    request_messages[clientsocket].put(m)
                    mode[clientsocket] = "writer"
                    state[clientsocket] = "data"
                    potential_writer.append(server)

                elif (tp == "ACK"):
                    print("%s: Recieve; ACK; Sequence:%d; Length:%d; Acknowledgement: %d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), int(ml[3].split(":")[1]), int(ml[4].split(":")[1])))
                    
                    seqno[clientsocket] = int(ml[3].split(":")[1])
                    client_win[clientsocket] = int(ml[4].split(":")[1])
                    mode[clientsocket] = "writer"
                    potential_writer.append(server)



        except io.BlockingIOError:
            print("wait input client")
            #print(len(potential_reader))
            #print(len(potential_writer))
            break




    #could be some possibilities recieving client first message



   
    if server in writable:
        
        print("in writable")
        print("writer: %d" % len(potential_writer))
        potential_writer.remove(server)
        #print(len(potential_writer))
        ##########################################
        #IN writable
        ##########################################
        
        for clientsocket in inputs:
            if mode[clientsocket] == "writer":
      
                m = request_messages[clientsocket].get_nowait()
                ml = m.split("\r\n")
                tp = ml[0]
             
                
                ##########################################
                    #try connection
                ##########################################
                if (state[clientsocket] == "connect"):
                    if (tp == "SYN"):
                        
                        ackno[clientsocket] += 1
                        print("%s: Send; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), 0, 0, ackno[clientsocket], buffer))
                        packet = "ACK\r\nSequence: " + str(0)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(ackno[clientsocket]) + "\r\n" + "Window:"+ str(buffer) + "\r\n"
                        packet = str.encode(packet)

                        server.sendto(packet, clientsocket)

                        #send syn from server

                        print("%s: Send; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), 0, length, ackno[clientsocket], buffer))

                        packet = "SYN\r\nSequence: " + str(0) + "\r\n" + "Length: "+str(length)+"\r\n"+ "Acknowledgement: " + str(ackno[clientsocket]) + "\r\n" + "Window: " + str(buffer) + "\r\n"
                        packet = str.encode(packet)

                        
                        #mode[server] = "writer"
                        mode[clientsocket] = "reader"

                        

                        server.sendto(packet, clientsocket)
                        
                ##########################################
                #transfer data
                ##########################################
                elif (state[clientsocket] == "data"):
                    print("data transfer in writable")
                    readfile = request_content[clientsocket][0].split(" ")[1][1:]
                    print("filename:"+readfile)
                    
                    
                    if (clientsocket not in file):
                        try:
                            f = open(readfile, 'rb')
                            file[clientsocket] = f
                            payload_size = min(client_win[clientsocket], payload)
                            filecontent = f.read(payload_size).decode()
                            request_messages[clientsocket].put(m)
                            server_win[clientsocket] -= len(filecontent)
                            
                        except FileNotFoundError:
                            print("file not found")
                    else:
                        print("client window: %d" % client_win[clientsocket])
                        print("server window: %d" % server_win[clientsocket])
                        print("payload: %d" % payload)
                        payload_size = min(client_win[clientsocket], payload)
                        filecontent = file[clientsocket].read(payload_size).decode()
                        request_messages[clientsocket].put(m)
                        print(filecontent)
                        server_win[clientsocket] -= len(filecontent)
                        
                    
                    print("%s: Send; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), seqno[clientsocket], len(filecontent), ackno[clientsocket], server_win[clientsocket]))
                    
                    HTTP_command = "HTTP/1.0 200 OK\r\n"
                    
                    if (request_content[clientsocket][1] == "Connection: keep-alive"):
                        HTTP_command += "Connection: keep-alive\r\n"
                    HTTP_command += "\r\n"
                        
                    packet = "DAT|ACK\r\nSequence: " + str(seqno[clientsocket]) + "\r\n" + "Length: "+str(len(filecontent))+"\r\n"+ "Acknowledgement: " + str(ackno[clientsocket]) + "\r\n" + "Window: " + str(server_win[clientsocket]) + "\r\n" + "\r\n" + HTTP_command + filecontent
                    
                    packet = str.encode(packet)
                    
                    mode[clientsocket] = "reader"
                    server.sendto(packet, clientsocket)
                   
                    
            ####################  

                    #print(len(potential_reader))
                
     
   
        
 
    if not (readable or writable or exceptional):
        #if timeout, then we need to send the packet again, therefore, we add back the socket to potential_writer
            
            if (mode[server] == "reader"):
                continue
                '''
            elif (mode[clientsocket] == "reader"):
                continue
                '''
                
            else:
                for clientsocket in inputs:
                    if (mode[clientsocket] == "reader"):
                        
             
               
                        print("Timeout! Resending packet")
                        recv_data = 0
                     
                        recv_ack = []
                        recv_socket = []
                        second_data = 0
                        resend_data = 0
                        timeout = 1
            
           
        

server.close()