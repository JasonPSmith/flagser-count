#include <stdio.h>
#include <iostream>

#include "flagser-count.cpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(pycount, m) {

  m.doc() = "Python interface for flagser-count";

  m.def("run_flagser_count", [](int num_vertices, std::vector<std::vector<value_t>>& edges,
                                 bool max_simplices, bool containment,
                                 bool return_simplices, bool print, char* print_address, char* threads, bool transpose,
                                 bool out, char* out_address, bool binary, char* binary_address,
                                 bool set_max_dim, char* max_dim, bool vertices_todo, char* vertices_address,
                                 bool set_max_dim_print, char* max_dim_print, bool set_min_dim_print, char* min_dim_print,
                                 bool compressed, bool npy, char* indices_address, char* indptr_address,
                                 std::vector<uint32_t> indices, std::vector<uint32_t> indptr, bool set_est_max_dim, char* est_max_dim,
                                 bool edge_containment) {
    // Save std::cout status
    auto cout_buff = std::cout.rdbuf();

    std::vector<char*> argv;
    argv.push_back((char*)"blank");
    argv.push_back((char*)"--in-format");
    argv.push_back((char*)"pyflagser");

    argv.push_back((char*)"--size");
    char nvs[(((sizeof num_vertices) * CHAR_BIT) + 2)/3 + 2];
    char noe[(((sizeof edges.size()) * CHAR_BIT) + 2)/3 + 2];
    sprintf(nvs, "%d", num_vertices);
    argv.push_back(nvs);

    argv.push_back((char*)"--threads");
    argv.push_back(threads);

    if(set_max_dim){
        argv.push_back((char*)"--max-dim");
        argv.push_back(max_dim);
    }
    if(set_max_dim_print){
        argv.push_back((char*)"--max-dim-print");
        argv.push_back(max_dim_print);
    }
    if(set_min_dim_print){
        argv.push_back((char*)"--min-dim-print");
        argv.push_back(min_dim_print);
    }

    if (max_simplices) { argv.push_back((char*)"--max-simplices"); }
    if (transpose) { argv.push_back((char*)"--transpose"); }
    if (containment){
        argv.push_back((char*)"--containment");
        argv.push_back((char*)"true");
    }
    if (edge_containment){
        argv.push_back((char*)"--edge-containment");
        argv.push_back((char*)"true");
        argv.push_back((char*)"--edges");
        sprintf(noe, "%ld", edges.size());
        argv.push_back(noe);
    }
    if (return_simplices){
        argv.push_back((char*)"--return_simplices");
        argv.push_back((char*)"true");
    }
    if (print){
        argv.push_back((char*)"--print");
        argv.push_back(print_address);
    }
    if (out){
        argv.push_back((char*)"--out");
        argv.push_back(out_address);
    }
    if (binary){
        argv.push_back((char*)"--binary");
        argv.push_back(binary_address);
    }
    if (vertices_todo){
        argv.push_back((char*)"--vertices-todo");
        argv.push_back(vertices_address);
    }
    if (set_est_max_dim){
        argv.push_back((char*)"--est-max-dim");
        argv.push_back(est_max_dim);
    }

    argv.push_back((char*)"--python");
    argv.push_back((char*)"true");

    parameters_t parameters(argv.size(), argv.data());

    // Disable cout
    std::cout.rdbuf(nullptr);

    // Building the filtered directed graph
    if(!compressed){
        auto graph = directed_graph_t(num_vertices, transpose, max_simplices);
        for (auto& edge : edges) {
            graph.add_edge(edge[0], edge[1], parameters);
        }

        count_cells<directed_graph_t,directed_flag_complex_t>(graph,parameters);
    } else {
        if(npy){
            auto graph = csr_directed_graph_t<uint32_t>(num_vertices);
            graph.indices = indices;
            graph.indptr = indptr;
            count_cells<csr_directed_graph_t<uint32_t>,csr_directed_flag_complex_t<uint32_t>>(graph,parameters);
        } else {
            cnpy::NpyArray indices_file = cnpy::npy_load(indices_address);
            cnpy::NpyArray indptr_file = cnpy::npy_load(indptr_address);
            if(cnpy::get_dtype(indices_address)){
                auto graph = csr_directed_graph_t<uint64_t>(num_vertices);
                graph.add_edges(indices_file,indptr_file);
                count_cells<csr_directed_graph_t<uint64_t>,csr_directed_flag_complex_t<uint64_t>>(graph,parameters);
            } else {
                auto graph = csr_directed_graph_t<uint32_t>(num_vertices);
                graph.add_edges(indices_file,indptr_file);
                count_cells<csr_directed_graph_t<uint32_t>,csr_directed_flag_complex_t<uint32_t>>(graph,parameters);
            }
        }
    }
    // Re-enable again cout
    std::cout.rdbuf(cout_buff);

    py::dict output;
    output["euler"] = parameters.euler_characteristic;
    output["cell_counts"] = parameters.total_cell_count;
    if(parameters.max_simplices){output["max_cell_counts"] = parameters.total_max_cell_count;}
    if(parameters.print_containment){output["contain_counts"] = parameters.contain_counts[0];}
    if(parameters.return_simplices){output["simplices"] = parameters.simplex_lists[0];}
    if(parameters.print_edge_containment){
        py::dict py_dict_edges;
        for(auto const& edge : parameters.edge_dict){
            py_dict_edges[py::make_tuple(edge.first.first,edge.first.second)] = parameters.edge_contain_counts[0][edge.second];
        }
        output["edge_contain_counts"] = py_dict_edges;
    }
    return output;
  });
}
