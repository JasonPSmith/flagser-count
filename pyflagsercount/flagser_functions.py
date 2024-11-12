
import numpy as np
from pycount import run_flagser_count

def flagser_count(adjacency_matrix,max_simplices=False,containment=False,
                  return_simplices=False, print='',threads=8,transpose=False,out='',binary='',max_dim=-1,
                  vertices_todo='',max_dim_print=-1,min_dim_print=-1, compressed=False, est_max_dim=-1,
                  edge_containment=False):

    return run_flagser_count(adjacency_matrix.shape[0], np.transpose(np.array(np.nonzero(adjacency_matrix))),
                             max_simplices,containment,return_simplices,(len(print)>0),print,
                             str(threads),transpose,(len(out)>0),out,(len(binary)>0),binary,
                             (max_dim!=-1),str(max_dim),(len(vertices_todo)>0),vertices_todo,
                             (max_dim_print!=-1),str(max_dim_print),(min_dim_print!=-1),str(min_dim_print),
                             False, False,'', '', np.array([]),np.array([]),(est_max_dim!=-1),str(est_max_dim),
                             edge_containment)

def flagser_count_edges(num_vertices, edge_list, max_simplices=False,containment=False,
                  return_simplices=False, print='',threads=8,transpose=False,out='',binary='',max_dim=-1,
                  vertices_todo='',max_dim_print=-1,min_dim_print=-1, compressed=False, est_max_dim=-1,
                  edge_containment=False):

    return run_flagser_count(num_vertices, edge_list,
                             max_simplices,containment,return_simplices,(len(print)>0),print,
                             str(threads),transpose,(len(out)>0),out,(len(binary)>0),binary,
                             (max_dim!=-1),str(max_dim),(len(vertices_todo)>0),vertices_todo,
                             (max_dim_print!=-1),str(max_dim_print),(min_dim_print!=-1),str(min_dim_print),
                             False, False,'', '', np.array([]),np.array([]),(est_max_dim!=-1),str(est_max_dim),
                             edge_containment)

def flagser_count_csr(num_vertices, indices_address, indptr_address,max_simplices=False,containment=False,
                  return_simplices=False, print='',threads=8,transpose=False,out='',binary='',max_dim=-1,
                  vertices_todo='',max_dim_print=-1,min_dim_print=-1, est_max_dim=-1):

    return run_flagser_count(num_vertices, np.array([]),
                             max_simplices,containment,return_simplices,(len(print)>0),print,
                             str(threads),transpose,(len(out)>0),out,(len(binary)>0),binary,
                             (max_dim!=-1),str(max_dim),(len(vertices_todo)>0),vertices_todo,
                             (max_dim_print!=-1),str(max_dim_print),(min_dim_print!=-1),str(min_dim_print),
                             True, False, indices_address, indptr_address, np.array([]), np.array([]),
                             (est_max_dim!=-1),str(est_max_dim),False)

def flagser_count_csr_npy(num_vertices, indices, indptr, max_simplices=False,containment=False,
                  return_simplices=False, print='',threads=8,transpose=False,out='',binary='',max_dim=-1,
                  vertices_todo='',max_dim_print=-1,min_dim_print=-1, est_max_dim=-1):

    assert str(indices.dtype)[-2:] == '32' and str(indptr.dtype)[-2:] == '32', "Indices and indptr must be 32 bits"

    return run_flagser_count(num_vertices, np.array([]),
                             max_simplices,containment,return_simplices,(len(print)>0),print,
                             str(threads),transpose,(len(out)>0),out,(len(binary)>0),binary,
                             (max_dim!=-1),str(max_dim),(len(vertices_todo)>0),vertices_todo,
                             (max_dim_print!=-1),str(max_dim_print),(min_dim_print!=-1),str(min_dim_print),
                             True, True, '', '', indices, indptr,(est_max_dim!=-1),str(est_max_dim),
                             False)
