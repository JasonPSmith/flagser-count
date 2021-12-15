import numpy as np

def parse_flagser_output(in_file,threads):
    f = open(in_file,'r')
    all_vertices=[]
    final_thread_output=[-1]*threads
    m = 0
    for st in f:
        line = st.split(" ")
        if len(line) > 5 and line[1]==':':
            all_vertices.append(int(line[0]))
            final_thread_output[int(line[3])] = [int(line[i]) for i in range(5,len(line)-3)]
            m = max(m,len(line)-8)
    current_count = [0]*m
    for i in final_thread_output:
        if i != -1:
            for j in range(0,min(len(current_count),len(i))):
                current_count[j] = current_count[j]+i[j]
    return (all_vertices,current_count)
