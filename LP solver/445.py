import sys
import numpy as np

def main():
    lines = []

    lines = initialize(lines)
    non_basic_num = len(lines[0])###########################
    constraint_num = len(lines)-1###########################
    print("constraint_num: ", constraint_num)
    print("non_basic_num: ", non_basic_num)

    lines = change_to_dic(lines,non_basic_num)
    #print()
    #print(lines)
    #print()

    feasible = check_feasible(lines)

    #objective line
    #c = lines[0]#############################
    x = []
    c = []
    for i in range(0, constraint_num):
        c.append(0)
    for i in range(0, non_basic_num):
        c.append(lines[0][i])

    for i in range(1, non_basic_num+constraint_num+1):
        x.append(i)

    #print("c: ", c)
    #print("x: ", x)

    #constraint lines
    w = []
    b = []
    A = []
    for i in range(non_basic_num+1, non_basic_num+constraint_num+1):
        w.append(i)
        b_element = lines[1:][i-non_basic_num-1][-1]
        A_element = lines[1:][i-non_basic_num-1][:-1]

        b.append(b_element)
        A.append(A_element)

    for j in range(0, constraint_num):

        for k in range(0, len(A)):
            if (k == j):
                A[k].append(1)
            else:
                A[k].append(0)
    #print("w: ", w)
    #print("b: ", b)
    #print("A: ", A)

    #print()
    curr_basic = []
    curr_nonbasic = []
    for i in range(1, constraint_num+1):
        curr_nonbasic.append(i)
    for j in range(constraint_num+1, non_basic_num+constraint_num+1):
        curr_basic.append(j)
    #print("curr_basic: ", curr_basic)
    #print("curr_nonbasic: ", curr_nonbasic)
    #print()
    #make A with basic first, non-basic next
    new_order = []
    x = []
    for i in range(len(curr_nonbasic)+1, len(curr_basic) + len(curr_nonbasic) + 1):
        new_order.append(i-1)
        x.append(i)

    for i in range(1, len(curr_nonbasic) + 1):
        new_order.append(i-1)
        x.append(i)

    #print("new_order: ", new_order)#with index

    np_A = np.array(A)
    A = np_A[:, new_order]
    #print("New A: ", A)

    ###
    ###
    simplex_method(A,b,c,curr_basic,curr_nonbasic,x)

def simplex_method(A,b,c,B,N,x):
    print()
    print("A: ", A)
    print("b: ", b)
    print("c: ", c)
    print("B: ", B)
    print("N: ", N)
    print("x: ", x)
    print()
    ###########################################
    ##Compute initial value of x
    Ab = []
    #make sure all the basis is at the top first, seperate with nb
    for i in range(0, len(B)):

        Ab.append(A[:,i].tolist())

    Ab = np.array(Ab)
    #!!!next line have trouble if Ab is non invertable
    try:
        Ab_inverse = np.linalg.inv(Ab)
    except:
        print("this matrix is non-invertible")
        exit(-1)

    print("Ab: ", Ab)
    An = []
    for i in range(len(B), len(B) + len(N)):
        An.append(i)#pos in A for nonbasic

    An = (A[: ,An].tolist())#choose those pos in A

    An = np.array(An)
    print("An: ", An)
    print()
    #print("Ab_inverse: ", Ab_inverse)
    xb = np.dot(Ab_inverse, b)
    #print("xb: ", xb)
    xn = []
    for i in range(0, len(N)):
        xn.append(0)
    #print("xn: ", xn)

    ##########################
    ##check if all xb >=0, else, infeasible
    feasible = 1
    for item in xb:
        if (item < 0):
            feasible = -1
            print("this dic is infeasible, error!")
            exit(-1)
    history = -1
    times = 0
    while (1):
        print()
        print("in loop")
        ######################################
        ##part1: compute z and check for optimality
        zb = []
        for i in (0, len(B)):
            zb.append(0)
        #print("zb: ", zb)

        #print("c: ", c)
        cb = []
        for i in range(0, len(B)):
            cb.append(c[i])
        #print("cb: ", cb)

        cn = []
        for i in range(len(B), len(B)+len(N)):
            cn.append(c[i])
        #print("cn: ", cn)

        #print(np.dot(np.dot(Ab_inverse,An).transpose(), cb))
        zn = np.dot(np.dot(Ab_inverse,An).transpose(), cb) - cn
        #print("zn: ",zn)

        optimize = 1
        for item in zn:
            if (item < 0):
                optimize = 0
                break
        if (optimize == 1):
            print("this is optimal")
            cb = np.array(cb)
            result = np.dot(np.dot(cb.transpose(), Ab_inverse), b)
            #if (result > int(result)):
                #result = int(result) + 1
            print("result: %7g" % result)

            #print("xb: ", xb)
            #print("Ab: ", Ab)
            for i in range(0, len(N)):
                if (i+1) in N:
                    print("0")


            exit(1)

        #####################################################

        print("not optimal yet")
        #part2: choose entering variable
        #pivot rule choose largest coefficient rule
        largest = 0
        j = -1
        for i in range(0, len(zn)):
            if (-zn[i] > largest):
                largest = -zn[i]
                j = i
        print("entering pos: ", j)  #0

        #so we choose
        print("choose as entering variable: ", N[j])
        #as entering variable

        ##############
        #part3 Choose leaving variable
        xb = np.dot(Ab_inverse, b)
        delta_xb = np.dot(Ab_inverse, A[:, j+len(B)])
        #print("delta_xb: ", delta_xb)
        delta_xn = []
        for i in range(0, len(N)):
            delta_xn.append(0)
        #print("xb: ", xb)
        t = sys.maxsize
        chosen = -1

        #print("xb: ", xb)
        #print("delta_xb: ", delta_xb)
        #print("Ab_inverse: ", Ab_inverse)
        #print("Aj: ",A[:, j+len(B)])

        for i in range(0, len(B)):
            if (delta_xb[i] != 0):
                current = float(xb[i]/delta_xb[i])

                if (t > current):
                    chosen = i
                    t = current
        #print("t: ",t)
        print("leaving variable: ", B[chosen])#leaving variable: pos in B
        if (history == B[chosen]):
            times += 1
        else:
            history = B[chosen]
            times = 1
        if (times == 3):
            print("may be cycle, break")
            exit(-1)
        i = chosen

        #############
        #part4: update for next iteration
        xb = xb - t*delta_xb
        #print("new xb: ", xb)
        xj = t
        #print("new xj: ", xj)
        set_j = N[j]
        set_i = B[i]
        #print("set_j: ", set_j)#1
        #print("set_i: ", set_i)#4


        index_n = x.index(set_j)
        copy_n = c[index_n]
        index_b = x.index(set_i)
        copy_b = c[index_b]


        #print("index_n: ", index_n)
        #print("index_b: ", index_b)
        #print("copy_n: ", copy_n)
        #print("copy_b: ", copy_b)

        c.pop(index_b)
        #print("c: ", c)
        c.insert(len(B)-1, copy_n)
        #print("c: ", c)
        c.pop(index_n)
        c.append(copy_b)
        #print("c after: ", c)

        #np.delete(a, row/column num, axis)#axis 0 row, 1 column
        #np.append(a, content, axis)#axis 0 row, 1 column
        #get column [:, num]
        #np.insert(arr, index, values, axis=None)
        #print(index_n)
        #print(index_b)
        copy_n = A[: ,index_n]
        copy_b = A[: ,index_b]
        #print("copy_n: ", copy_n)
        #print("copy_b: ", copy_b)


        A = np.delete(A, index_b, axis = 1)
        #numpy.reshape(a, newshape, order='C')

        temp = []
        for item in copy_n:
            temp.append([item])
        A = np.insert(A, len(B) - 1, copy_n, axis = 1)


        A = np.delete(A, index_n, axis = 1)
        temp = []
        for item in copy_b:
            temp.append([item])
        A = np.append(A, temp, axis = 1)
        #print("A: ", A)

        Ab = []
        #make sure all the basis is at the top first, seperate with nb
        for i in range(0, len(B)):

            Ab.append(A[:,i])

        Ab = np.array(Ab)
        Ab = A[0:len(B),0:len(B)]
        print("Ab: ", Ab)
        #!!!next line have trouble if Ab is non invertable
        try:
            Ab_inverse = np.linalg.inv(Ab)
        except:
            print("this matrix is non-invertible")
            exit(-1)


        An = []
        for i in range(len(B), len(B) + len(N)):
            An.append(i)#pos in A for nonbasic

        An = (A[: ,An].tolist())#choose those pos in A

        An = np.array(An)
        print("An: ", An)


        B.append(set_j)
        B.remove(set_i)
        N.append(set_i)
        N.remove(set_j)
        print("B: ", B)
        print("N: ", N)
        x = B + N
        print("x: ", x)

        break

        #print()












def change_to_dic(lines, non_basic_num):
    for pos in range(0, non_basic_num):
        lines[0][pos] = float(lines[0][pos])
    for line in lines[1:]:
        for pos in range (0,non_basic_num):
            line[pos] = float(line[pos])
        line[non_basic_num] = float(line[non_basic_num])
    return lines

def initialize(lines):

    for line in sys.stdin.readlines():
        line = line.strip('\n')
        line = line.split()
        lines.append(line)
    return lines

def check_feasible(lines):
    #check feasibility
    feasible = 1

    for ls in lines[1:]:
        if (ls[-1] < 0):
            #print("infeasible")
            feasible = 0
            return 0

    #print("feasible")
    return 1

if __name__ == "__main__":
    main()
