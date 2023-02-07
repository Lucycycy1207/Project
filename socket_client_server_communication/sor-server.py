import sys
import socket
import select
import time
import queue
import io
#import os

command = ""
value = 0

x = 0

server=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server.setblocking(0)

ip = str(sys.argv[1])
port = int(sys.argv[2])
buffer = int(sys.argv[3])
payload = int(sys.argv[4])

server.bind((ip,port))


uncheck_data = {}

potential_reader=[server]
potential_writer=[]
potential_exp=[]


recv_socket = []


mode = {}

mode[server] = "reader"
state = {}#connect,

inputs = []

seqno = {}#next seqno
ackno={}

client_win = {}
server_win = {}

request_respond = {}#request_respond pair
request_messages = {}
request_content = {}
file = {}

while True:
    readable, writable, exceptional = select.select(potential_reader,potential_writer, potential_exp, 1)
    #print("reader:%d" % len(potential_reader))
    #print("writer:%d" % len(potential_writer))


    if server in readable:
        try:

            m, clientsocket = server.recvfrom(buffer)
            m = m.decode()
            ml = m.split("\r\n")
            tp = ml[0]
            new_info = 1

        except io.BlockingIOError:
            new_info = 1
            #print("wait input client")
            if (mode[server] == "reader"):#no input yet
                continue
        #print("in readable")
        #print("reader: %d" % len(potential_reader))

        if clientsocket not in inputs:
            state[clientsocket] = "connect"
            inputs.append(clientsocket)

        if (state[clientsocket] == "connect"):

            if (tp == "SYN"):
                client_seq = int(ml[1].split(":")[1])
                client_length = int(ml[2].split(":")[1])
                client_ack = int(ml[3].split(":")[1])
                client_win[clientsocket] = int(ml[4].split(":")[1])
                #print("%s: Recieve; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), client_seq, client_length, client_ack, client_win[clientsocket]))
                request_respond[clientsocket] =[]
                request_respond[clientsocket].append([ml,""])

                server_win[clientsocket] = buffer
                ackno[clientsocket] = 0

                mode[clientsocket] = "writer"
                request_messages[clientsocket] = queue.Queue()
                request_messages[clientsocket].put(m)
                potential_writer.append(server)

            elif (tp == "ACK"):
                #print("%s: Recieve; ACK; Sequence:%d; Length:%d; Acknowledgement: %d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), int(ml[3].split(":")[1]), int(ml[4].split(":")[1])))

                seqno[clientsocket] = (int(ml[3].split(":")[1]))
                client_win[clientsocket] = int(ml[4].split(":")[1])

                uncheck_data[clientsocket].remove(uncheck_data[clientsocket][0])

                state[clientsocket] = "data"
                mode[clientsocket] = "reader"

        ########################
        #recieve data from client
        #######################
        elif (state[clientsocket] == "data"):
            if (tp == "DAT|ACK"):
                #print("recive data from client!")
                #print("%s: Recieve; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), ackno[clientsocket], int(ml[4].split(":")[1])))
                #it may have multiple requests here
                seqno[clientsocket] = int(ml[3].split(":")[1])
                ackno[clientsocket] += int(ml[2].split(":")[1])
                client_win[clientsocket] = int(ml[4].split(":")[1])

                request_content[clientsocket] = ml[6:-2]
                #print(ml[6:-2])
                tmp = ""
                for i in range(0, len(ml[6:-2])):
                    if (i == len(ml[6:-2]) -1):
                        tmp += ml[6:-2][i]
                    else:
                        tmp += ml[6:-2][i]+";"

                print("%s: %s %d %s" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), clientsocket[0],clientsocket[1], tmp))
                request_messages[clientsocket].put(m)
                mode[clientsocket] = "writer"
                state[clientsocket] = "data"
                potential_writer.append(server)

            elif (tp == "ACK"):

                #print("%s: Recieve; ACK; Sequence:%d; Length:%d; Acknowledgement: %d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), int(ml[1].split(":")[1]), int(ml[2].split(":")[1]), int(ml[3].split(":")[1]), int(ml[4].split(":")[1])))

                seqno[clientsocket] = int(ml[3].split(":")[1])
                client_win[clientsocket] = int(ml[4].split(":")[1])
                mode[clientsocket] = "writer"

                for data in uncheck_data[clientsocket]:
                    tmp = int(data.split("\r\n")[1].split(":")[1]) + int(data.split("\r\n")[2].split(":")[1])
                    #if (tmp == seqno[clientsocket]):

                        #print("this data got the ack back %d" % tmp)

                uncheck_data[clientsocket].remove(uncheck_data[clientsocket][0])

                potential_writer.append(server)




    if server in writable:

        #print("in writable")
        #print("writer: %d" % len(potential_writer))
        potential_writer.remove(server)
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
                        #print("%s: Send; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), 0, 0, ackno[clientsocket], buffer))
                        packet = "ACK\r\nSequence: " + str(0)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(ackno[clientsocket]) + "\r\n" + "Window:"+ str(buffer) + "\r\n"
                        packet = str.encode(packet)

                        server.sendto(packet, clientsocket)

                        #send syn from server

                        #print("%s: Send; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), 0, 0, ackno[clientsocket], buffer))

                        packet = "SYN\r\nSequence: " + str(0) + "\r\n" + "Length: "+str(0)+"\r\n"+ "Acknowledgement: " + str(ackno[clientsocket]) + "\r\n" + "Window: " + str(buffer) + "\r\n"
                        packet = str.encode(packet)
                        uncheck_data[clientsocket] = []
                        uncheck_data[clientsocket].append(packet)

                        #mode[server] = "writer"
                        mode[clientsocket] = "reader"

                        server.sendto(packet, clientsocket)
                ##########################################
                #transfer data
                ##########################################
                elif (state[clientsocket] == "data"):
                    #print("data transfer in writable")
                    readfile = request_content[clientsocket][0].split(" ")[1][1:]
                    #print("filename:"+readfile)

                    while (client_win[clientsocket] != 0):
                        no_file = 0
                        if (clientsocket not in file):
                            try:
                                f = open(readfile, 'rb')
                                file[clientsocket] = f
                                payload_size = min(client_win[clientsocket], payload)
                                filecontent = f.read(payload_size)
                                if (filecontent == -1):
                                    file[clientsocket].close()
                                    exit()
                                else:
                                    filecontent = filecontent.decode()
                                request_messages[clientsocket].put(m)
                                client_win[clientsocket] -= len(filecontent)

                            except FileNotFoundError:
                                no_file = 1
                                #print(readfile)
                                print("file not found")
                        else:
                            payload_size = min(client_win[clientsocket], payload)
                            filecontent = file[clientsocket].read(payload_size).decode()
                            request_messages[clientsocket].put(m)

                            client_win[clientsocket] -= len(filecontent)


                        if (no_file == 1):
                            #print("%s: Send; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), seqno[clientsocket], 0, ackno[clientsocket], server_win[clientsocket]))

                            HTTP_command = "HTTP/1.0 404 Not Found\r\n"
                        else:
                            #print("%s: Send; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), seqno[clientsocket], len(filecontent), ackno[clientsocket], server_win[clientsocket]))

                            HTTP_command = "HTTP/1.0 200 OK\r\n"

                        if (request_content[clientsocket][1] == "Connection: keep-alive"):
                            HTTP_command += "Connection: keep-alive\r\n"
                        else:
                            file[clientsocket].close()
                        HTTP_command += "\r\n"

                        if (no_file != 1):
                            packet = "DAT|ACK\r\nSequence: " + str(seqno[clientsocket]) + "\r\n" + "Length: "+str(len(filecontent))+"\r\n"+ "Acknowledgement: " + str(ackno[clientsocket]) + "\r\n" + "Window: " + str(server_win[clientsocket]) + "\r\n" + "\r\n" + HTTP_command + filecontent
                            seqno[clientsocket] += len(filecontent)
                        else:
                            packet = "DAT|ACK\r\nSequence: " + str(seqno[clientsocket]) + "\r\n" + "Length: "+str(0)+"\r\n"+ "Acknowledgement: " + str(ackno[clientsocket]) + "\r\n" + "Window: " + str(server_win[clientsocket]) + "\r\n" + "\r\n" + HTTP_command

                        uncheck_data[clientsocket].append(packet)
                        packet = str.encode(packet)


                        server.sendto(packet, clientsocket)

                    mode[clientsocket] = "reader"


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
