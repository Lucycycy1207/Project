import sys
import socket
import select
import time
#import os

command = ""
value = 0

##get input file socket info
'''
readfile = sys.argv[3]
writefile = sys.argv[4]
'''
###################################
ip = str(sys.argv[1])
port = int(sys.argv[2])
buffer = int(sys.argv[3])
payload = int(sys.argv[4])
file_list = sys.argv[5:]
file_list_pos = 0
'''
print("ip_address:"+ip)
print("port num:%d" % port)
print("buffer:%d" % buffer)
print("payload:%d" % payload)
for i in range (0, len(file_list)):
    print("file:"+ file_list[i])
'''
##########################################
#f = open(readfile, "r")
x = 0

#create a TCP/IP socket
clientsocket=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#set the socket to non-blocking mode
clientsocket.setblocking(0)

clientsocket.bind(("192.168.1.100",8888))


'''
1. need to use a queue to temporarily store the content of each received 
packet
2.When reaching an end of an ACK or DATA, extract all content from the queue 
and process the content line by line
'''
buffer = 2048



#first packet

#reciever
#SYN|FIN|DAT|ACK|RST, 
#check if SYN, send synack
uncheck_data = []

potential_reader=[]
potential_writer=[clientsocket]
potential_exp=[clientsocket]

connection_status = 0
data_section = 0


packet = ""
conn_tp = "SYN" 
fin_tp = "FIN"



#f = open(readfile, 'rb')
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

seqno = 0
length = 0 
finish = 0
msgls = ["ACK"]
sending_ack = 0
payload = []

###############
ackno = -1
mode = "writer"
client_win = buffer
write = 0
data_tp = "DAT"
while True:
    

    readable, writable, exceptional = select.select(potential_reader,potential_writer, potential_exp, 1)
    #print("reader:%d" % len(potential_reader))
    #print("writer:%d" % len(potential_writer))
    if clientsocket in writable:   
        print("in writable")
       
        print("potential_writer: %d" % len(potential_writer))
        ##########################################
        #IN writable
        ##########################################
        if (connection_status == 0):
        ##########################################
        #try connection
        ##########################################
####################         
            if (timeout ==1 ):
                timeout = 0
                conn_tp == "SYN"

            if (conn_tp == "SYN"):
                #"SYN|DAT|ACK|FIN"
                #"SYN|DAT|ACK"
                #"SYN"
                print("%s: Send; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), seqno, length, ackno, buffer))
                
                packet = "SYN\r\nSequence: " + str(seqno) + "\r\n" + "Length: "+str(length)+"\r\n"+ "Acknowledgement: " + str(-1) + "\r\n" + "Window: " + str(buffer) + "\r\n"

                if packet not in uncheck_data:
                    uncheck_data.append(packet)
              
                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip,port))
                
                if (len(potential_reader) == 0):
                    potential_reader.append(clientsocket)
                potential_writer.remove(clientsocket) 
              
                
                


            elif (conn_tp == "ACK"):
                
                print("%s: Send; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), 0, 0, ackno, buffer))
                packet = "ACK\r\nSequence: " + str(0)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(ackno) + "\r\n" + "Window:"+ str(buffer) + "\r\n"
                
                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip,port))
              
                connection_status = 1
                data_section = 1
                
                
        elif (data_section == 1):
            
            if (data_tp == "DAT"):
                print("client send data")
                command = "GET " + "/" + file_list[file_list_pos] + " "+"HTTP/1.0\r\n"+"Connection: keep-alive\r\n\r\n"
                packet = "DAT\r\nSequence: " + str(seqno) + "\r\n" + "Length: "+str(len(command))+"\r\n"+ "Acknowledgement: " + str(ackno) + "\r\n" + "Window: " + str(buffer) + "\r\n" + "\r\n" + command

                print("%s: Send; DAT; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), seqno, len(command), ackno, buffer))

                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip, port))


                potential_writer.remove(clientsocket)
                potential_reader.append(clientsocket)
                
            elif (data_tp == "ACK"):
                print("%s: Send; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), seqno, 0, ackno, client_win))
                packet = "ACK\r\nSequence: " + str(seqno)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(ackno) + "\r\n" + "Window:"+ str(client_win) + "\r\n"
                
                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip,port))
                potential_writer.remove(clientsocket)
                potential_reader.append(clientsocket)
                break
              
          
              
 

    if clientsocket in readable:
        
        print("in readable")
        print("potential_reader: %d" % len(potential_reader))
    ##########################################
    #IN writable
    ##########################################
        m = clientsocket.recv(1460)#1024+header size
        m = m.decode()
        
    
        
        msgls = m.split("\r\n")
        tp = msgls[0]
        ################
        
        #################
        if (connection_status == 0):
        ##########################################
        #try connection
        ##########################################

            if (tp == "SYN"):
                #print("syn,return ack,window")
               
                print("%s: Recieve; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(msgls[1].split(":")[1]), int(msgls[2].split(":")[1]), int(msgls[3].split(":")[1]), int(msgls[4].split(":")[1])))
                server_win = int(msgls[4].split(":")[1])
                conn_tp = "ACK"
                ackno = int(msgls[1].split(":")[1]) + 1
                potential_writer.append(clientsocket) 
                potential_reader.remove(clientsocket)
                

            elif (tp == "ACK"):
                client_win = int(m.split("\r\n")[4].split(":")[1])
                win2 = client_win
                #print(msgls)
                
                print("%s: Recieve; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(msgls[1].split(":")[1]), int(msgls[2].split(":")[1]), int(msgls[3].split(":")[1]), int(msgls[4].split(":")[1])))
#########################
                #msg = "ACK\r\nAcknowledgement: "+ str(seqno) + "\r\n" + "Window:"+ str(client_win) + "\r\n"
                seqno = int(msgls[3].split(":")[1])
                #pre_recv_ack = msg
#########################
                for i in range (0, len(uncheck_data)):
                    if (int(uncheck_data[i].split("\r\n")[1].split(":")[1]) + 1 == seqno):
                        uncheck_data.remove(uncheck_data[i])
                tp == "SYN"
                print("potential_reader: %d" % len(potential_reader))
                
                mode = "reader"
                '''
                data_section = 1
                connection_status = 1
                '''
                #exit()
            
            #potential_writer.append(clientsocket) 
          
        #print(len(potential_reader))   
        elif (data_section == 1):
            print("recieve data from server")
            
            server_seqno = int(msgls[1].split(":")[1])
            server_length = int(msgls[2].split(":")[1])
            server_ackno = int(msgls[3].split(":")[1])
            server_win = int(msgls[4].split(":")[1])
            
            print("%s: Recieve; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), server_seqno, server_length, server_ackno, server_win))
            
            seqno = server_ackno
            if (ackno == server_seqno):#inorder packet
                seqno = server_ackno
                server_content = msgls[6:]
                client_win -= server_length

                if (server_content[0] == "HTTP/1.0 200 OK"):
                    if (server_content[1] == "Connection: keep-alive"):#wait for further content

                        if write == 0:

                            f = open(file_list[file_list_pos+1], 'w')
                            f.write(server_content[3])
                            write = 1

                            potential_writer.append(clientsocket)
                            potential_reader.remove(clientsocket)
                            data_tp = "ACK"
                            ackno += server_length


                        else:
                            f = open(file_list[file_list_pos+1], 'a')
                            f.write(server_content[3])
                        f.close()
                    else:
                        print("connection close")

                else:#filename not found
                    print("file not found")
                    if (len(file_list) == 2):
                        print("FIN")
                    else:
                        for i in range (0, len(file_list) -2):
                            file_list[i] = file_list[i+2]
                            file_list.pop(-1)
                    exit()
        
 
    if not (readable or writable or exceptional):
        #if timeout, then we need to send the packet again, therefore, we add back the socket to potential_writer
        
        print("Timeout! Resending packet")
        recv_data = 0
        payload = []
        recv_ack = []
        recv_socket = []
        second_data = 0
        resend_data = 0
        timeout = 1

        potential_writer.append(clientsocket)


clientsocket.close()