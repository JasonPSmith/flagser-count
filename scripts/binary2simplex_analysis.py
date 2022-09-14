#Converts from binary format given by flagser when using --binary to a list of simplices
import numpy as np

def simplexanalysis(T,S):
    #DO SOME ANALYSIS OF YOU SIMPLEX HERE
    #WHERE T IS YOUR CURRENT SIMPLEX
    #AND S IS FOR STORING YOUR OUTPUT VALUES


#INPUT: Address of binary file storing simplices
#OUTPUT: A list if lists L where L[i] contains the vertex ids of the i'th simplex,
#          note the simplices appear in no particular order
def binary2simplex(address):
    X = np.fromfile(address, dtype='uint64')                         #Load binary file
    S=[]                                                             #Initialise empty list for simplices

    i=0
    while i < len(X):
        b = format(X[i], '064b')                                     #Load the 64bit integer as a binary string
        if b[0] == '0':                                              #If the first bit is 0 this is the start of a new simplex
            if i != 0:                                               
                simplexanalysis(T,S)                                 #Does the analysis of the simplices    
            T = []
        t=[int(b[-21:],2), int(b[-42:-21],2), int(b[-63:-42],2)]     #Compute the 21bit ints stored in this 64bit int
        for j in t:
            if j != 2097151:                                         #If an int is 2^21 this means we have reached the end of the simplex, so don't add it
                T.append(j)
        i+=1

    simplexanalysis(T,S)                                             #This analyses the final simplex in the list
    return S