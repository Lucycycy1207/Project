import sys
import socket
import select
import time


ip = str(sys.argv[1])
port = int(sys.argv[2])
buffer = int(sys.argv[3])
payload = int(sys.argv[4])
file_list = sys.argv[5:]
file_list_pos = 0


clientsocket=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
clientsocket.setblocking(0)
#clientsocket.bind(("192.168.56.1",8887))
clientsocket.bind(("192.168.1.100",8888))

uncheck_data = []

potential_reader=[]
potential_writer=[clientsocket]
potential_exp=[clientsocket]

connection_status = 0
data_section = 0
conn_tp = "SYN"
client_win = buffer
write = 0
data_tp = "DAT"
send_message = []
timeout = 0
connection_end = 0


while True:
    readable, writable, exceptional = select.select(potential_reader,potential_writer, potential_exp, 1)
    #print("reader:%d" % len(potential_reader))
    #print("writer:%d" % len(potential_writer))
    if clientsocket in writable:
        #print("in writable")
        #print("potential_writer: %d" % len(potential_writer))

        if (connection_status == 0):
        ##########################################
        #connection
        ##########################################
            if (timeout == 1):
                timeout = 0
                conn_tp == "SYN"

            if (conn_tp == "SYN"):
                client_seq = 0
                client_ack = -1
                length = 0
                print("%s: Send; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), client_seq, length, client_ack, buffer))
                packet = "SYN\r\nSequence: " + str(client_seq) + "\r\n" + "Length: "+str(length)+"\r\n"+ "Acknowledgement: " + str(client_ack) + "\r\n" + "Window: " + str(buffer) + "\r\n"

                if packet not in uncheck_data:
                    uncheck_data.append(packet)

                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip,port))

                potential_reader.append(clientsocket)
                potential_writer.remove(clientsocket)

            elif (conn_tp == "ACK"):
                print("%s: Send; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), client_seq, 0, client_ack, buffer))
                packet = "ACK\r\nSequence: " + str(client_seq)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(client_ack) + "\r\n" + "Window:"+ str(buffer) + "\r\n"
                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip,port))

                connection_status = 1
                data_section = 1
        #############################################
        #END CONNECTION
        #############################################
        elif (data_section == 1):

            if (data_tp == "DAT"):
                #print("client send data")

                command = "GET " + "/" + file_list[file_list_pos] + " "+"HTTP/1.0\r\n"+"Connection: keep-alive\r\n\r\n"
                file_list_pos +=1

                packet = "DAT|ACK\r\nSequence: " + str(client_seq) + "\r\n" + "Length: "+str(len(command))+"\r\n"+ "Acknowledgement: " + str(client_ack) + "\r\n" + "Window: " + str(buffer) + "\r\n" + "\r\n" + command

                print("%s: Send; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), client_seq, len(command), client_ack, buffer))
                client_seq += len(command)
                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip, port))


                potential_writer.remove(clientsocket)
                potential_reader.append(clientsocket)

            elif (data_tp == "ACK"):
                print("%s: Send; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), client_seq, 0, client_ack, client_win))
                packet = "ACK\r\nSequence: " + str(client_seq)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(client_ack) + "\r\n" + "Window:"+ str(client_win) + "\r\n"

                packet = str.encode(packet)
                clientsocket.sendto(packet, (ip,port))
                potential_writer.remove(clientsocket)
                potential_reader.append(clientsocket)
                #print("end reader: %d" % len(potential_reader))
                #print("end writer: %d" % len(potential_writer))
        elif (connection_end == 1):
            print("%s: Send; FIN|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), client_seq, length, client_ack, buffer))
            packet = "FIN|ACK\r\nSequence: " + str(client_seq) + "\r\n" + "Length: "+str(0)+"\r\n"+ "Acknowledgement: " + str(client_ack) + "\r\n" + "Window: " + str(buffer) + "\r\n"
            packet = str.encode(packet)
            clientsocket.sendto(packet,(ip,port))
            potential_writer.remove(clientsocket)
            potential_reader.append(clientsocket)


    if clientsocket in readable:

        #print("in readable")
        #print("potential_reader: %d" % len(potential_reader))
    ##########################################
    #IN writable
    ##########################################
        try:
            m = clientsocket.recv(1460)#1024+header size
            m = m.decode()
            msgls = m.split("\r\n")
            tp = msgls[0]
        except io.BlockingIOError:
            #print("laaaaa")
            continue

        if (connection_status == 0):
        ##########################################
        #connection
        ##########################################
            if (tp == "SYN"):
                server_seq = int(msgls[1].split(":")[1])
                server_len = int(msgls[2].split(":")[1])
                server_ack = int(msgls[3].split(":")[1])
                server_win = int(msgls[4].split(":")[1])
                print("%s: Recieve; SYN; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), server_seq, server_len, server_ack, server_win))
                conn_tp = "ACK"
                client_ack = server_seq + 1
                potential_writer.append(clientsocket)
                potential_reader.remove(clientsocket)

            elif (tp == "ACK"):
                server_win = int(msgls[4].split(":")[1])
                server_seq = int(msgls[1].split(":")[1])
                length = int(msgls[2].split(":")[1])
                server_ack = int(msgls[3].split(":")[1])
                print("%s: Recieve; ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d" % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), server_seq, length, server_ack, server_win))

                client_seq = server_ack
                for i in range (0, len(uncheck_data)):
                    tmp_seq = int(uncheck_data[i].split("\r\n")[1].split(":")[1])
                    tmp_length = int(uncheck_data[i].split("\r\n")[2].split(":")[1])
                    if (tmp_seq+tmp_length == server_ack):
                        uncheck_data.remove(uncheck_data[i])
                        break
        ##########################################
        #END connection
        ##########################################

        #print(len(potential_reader))
        elif (data_section == 1):
            #print("recieve data from server")

            server_seq = int(msgls[1].split(":")[1])
            server_length = int(msgls[2].split(":")[1])
            server_ackno = int(msgls[3].split(":")[1])
            server_win = int(msgls[4].split(":")[1])

            print("%s: Recieve; DAT|ACK; Sequence:%d; Length:%d; Acknowledgement:%d; Window:%d"
            % (time.strftime("%a %b %d %H:%M:%S PDT %Y"), server_seq, server_length, server_ackno, server_win))
            client_ack += server_length
            client_win -= server_length
            if (file_list_pos == len(file_list)/2):
                connection_end = 1
                #print(len(potential_writer))
                continue
            else:
                if (server_win != 0):
                    potential_writer.append(clientsocket)
                potential_reader.remove(clientsocket)

            #packet = "ACK\r\nSequence: " + str(server_seqno)+"\r\n"+"Length: " + str(0) + "\r\n" + "Acknowledgement:"+ str(server_ackno) + "\r\n" + "Window:"+ str(client_win) + "\r\n"


            seqno = server_ackno
            if (client_ack == server_seq):#inorder packet
                seqno = server_ackno
                server_content = msgls[6:]
                client_win -= server_length

                if (server_content[0] == "HTTP/1.0 200 OK"):
                    if (server_content[1] == "Connection: keep-alive"):#wait for further content

                        if write == 0:

                            f = open(file_list[file_list_pos+1], 'w')
                            f.write(server_content[3])
                            write = 1


                            data_tp = "ACK"
                            client_ack += server_length


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

            elif(connection_end == 1):

                exit()

    if not (readable or writable or exceptional):
        #if timeout, then we need to send the packet again, therefore, we add back the socket to potential_writer

        #print("Timeout! Resending packet")
        recv_data = 0
        payload = []
        recv_ack = []
        recv_socket = []
        second_data = 0
        resend_data = 0
        timeout = 1

        potential_writer.append(clientsocket)


clientsocket.close()
