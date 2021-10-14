from subprocess import Popen, PIPE, STDOUT
import os
exec(open('../scripts/binary2simplex.py').read())
#Test example A: checkes basic flagser-count with flagser format
p = Popen('../flagser-count --in A.flag',shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
out, err = p.communicate()
try:
    if(out.splitlines()[-1].decode("utf-8") == '-1 5 7 1'):
        print("Example A Correct")
    else:
        print("ERROR: Problem with Example A")
except:
    print("Something went wrong with Example A")
    print(err)

#Test example B: checks edge-list format, out function, print function and parallel threads
if os.path.exists("B.out"):
  os.remove("B.out")
p = Popen('../flagser-count --in-format edge-list --in B.edges --size 5 --threads 2 --out B.out --print B',shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
out, err = p.communicate()
try:
    if(list(open('B.out'))[-1][:-1] == '2 5 9 6') and set([i[:-2] for i in open('B0.simplices')]).union(set([i[:-2] for i in open('B1.simplices')])) == {'3 0 1', '3 1 2', '4 1 2', '3 2 0', '4 0 1', '4 2 0'}:
        print("Example B Correct")
    else:
        print("ERROR: Problem with Example B")
except:
    print("Something went wrong with Example B")
    print(err)

#Test example C: checks coo format,containment and binary print.
p = Popen('../flagser-count --in-format coo --row C_row.npy --col C_col.npy --size 5 --threads 2 --binary C --containment C.cliques',shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
out, err = p.communicate()
try:
    C = set([tuple(i) for i in binary2simplex('C0.binary')]).union(set([tuple(i) for i in binary2simplex('C1.binary')]))
    bin_bool = C == {(3, 0, 2), (4, 0, 1), (4, 0, 1, 2), (4, 0, 2), (0, 1, 2), (3, 0, 1, 2), (3, 0, 1), (4, 1, 2), (3, 1, 2)}
    contain_bool = list(open('C.cliques')) == ['1 4 5 2 \n', '1 4 5 2 \n', '1 4 5 2 \n', '1 3 3 1 \n', '1 3 3 1 \n']
    if out.splitlines()[-1].decode("utf-8") == '1 5 9 7 2' and bin_bool and contain_bool:
        print("Example C Correct")
    else:
        print("ERROR: Problem with Example C")
except:
    print("Something went wrong with Example C")
    print(err)

#Test example D: checks csc format and transpose
p = Popen('../flagser-count --in-format csc --indices D_indices.npy --indptr D_indptr.npy --size 7 --transpose',shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
out, err = p.communicate()
try:
    if out.splitlines()[-1].decode("utf-8") == '1855 7 42 210 840 2520 5040 5040':
        print("Example D Correct")
    else:
        print("ERROR: Problem with Example D")
except:
    print("Something went wrong with Example D")
    print(err)

#Test example D as 64bit: checks csc format and transpose
p = Popen('../flagser-count --in-format csc --indices D_indices64.npy --indptr D_indptr64.npy --size 7 --transpose',shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
out, err = p.communicate()
try:
    if out.splitlines()[-1].decode("utf-8") == '1855 7 42 210 840 2520 5040 5040':
        print("Example D (64bit) Correct")
    else:
        print("ERROR: Problem with Example D (64bit)")
except:
    print("Something went wrong with Example D (64bit)")
    print(err)

