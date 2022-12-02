#ifndef FLAGSER_INPUT_H
#define FLAGSER_INPUT_H

#include "definitions.h"
#include "argparser.h"
#include "definitions.h"
#include "directed_graph.h"

//#############################################################################
//Functions for parsing strings
inline std::string trim(const std::string& s) {
    auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) { return std::isspace(c); });
    auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) { return std::isspace(c); }).base();
    return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}
unsigned int string_to_uint(std::string s) { return atoi(s.c_str()); }
std::vector<vertex_index_t> split(const std::string& s, char delim, const std::function<vertex_index_t(std::string)>& transform) {
    std::vector<vertex_index_t> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) elems.push_back(transform(item));
    return elems;
}

//#############################################################################
//csc format
template <typename T,typename C> void read_graph_csc(T& graph, parameters_t& parameters) {
    //Load numpy arrays
    cnpy::NpyArray indices_file = cnpy::npy_load(parameters.input_address1);
    cnpy::NpyArray indptr_file = cnpy::npy_load(parameters.input_address2);
    std::vector<C> indices = indices_file.as_vec<C>();
    std::vector<C> indptr = indptr_file.as_vec<C>();
    for(int i = 0; i < indptr.size()-1; i++){
        for(int j = indptr[i]; j < indptr[i+1]; j++){
            graph.add_edge(indices[j],i,parameters);
        }
    }
}


//#############################################################################
//coo format
template <typename T,typename C> void read_graph_coo(T& graph, parameters_t& parameters) {

    cnpy::NpyArray row_file = cnpy::npy_load(parameters.input_address1);
    cnpy::NpyArray col_file = cnpy::npy_load(parameters.input_address2);
    std::vector<C> row = row_file.as_vec<C>();
    std::vector<C> col = col_file.as_vec<C>();

    if (row.size() != col.size()){
        std::cerr << "ERROR: Row and Column lengths do not match."<< std::endl;
        exit(-1);
    }

    for(int i = 0; i < row.size(); i++){ graph.add_edge(row[i],col[i],parameters); }
}

//#############################################################################
//flagser format
template <typename T> void read_graph_flagser(T& graph, parameters_t& parameters) {
    //Open input file
    std::string line;
    int current_dimension = 1;
    std::ifstream input_stream(parameters.input_address1);
    if (input_stream.fail()) {  std::cerr << "couldn't open file " << parameters.input_address1 << std::endl; exit(-1); }

    //If a dim 0 is in the file take the next line and use it to get the number of vertices
    //Otherwise take each line to be an edge.
    while (not input_stream.eof()) {
        getline(input_stream, line);
        line = trim(line);
        if (line.length() == 0) continue;
        if (line[0] == 'd' && line[1] == 'i' && line[2] == 'm' && line[4] == '0') { current_dimension = 0; continue; }
        if (line[0] == 'd' && line[1] == 'i' && line[2] == 'm' && line[4] == '1') { current_dimension = 1; continue; }
        if (current_dimension == 0) {
            graph.set_number_of_vertices(split(line, ' ', string_to_uint).size());
        } else {
            std::vector<vertex_index_t> vertices = split(line, ' ', string_to_uint);
            graph.add_edge(vertices[0], vertices[1], parameters);
        }
    }
}

//#############################################################################
//csr compressed format
template <typename T> void read_graph_csr_compressed(T& graph, parameters_t& parameters) {
    //Load numpy arrays
    cnpy::NpyArray indices_file = cnpy::npy_load(parameters.input_address1);
    cnpy::NpyArray indptr_file = cnpy::npy_load(parameters.input_address2);
    graph.add_edges(indices_file,indptr_file);
}


//#############################################################################
//Base class

template <typename T> void read_directed_graph(T& graph, parameters_t& parameters) {
    std::cout <<  "Reading in the graph..."<< std::flush;

    if (parameters.input_format == "flagser") { read_graph_flagser<T>(graph, parameters); }
    else if (parameters.input_format == "csc") {
        if(parameters.type) read_graph_csc<T,uint64_t>(graph, parameters);
        else read_graph_csc<T,uint32_t>(graph, parameters);
    }
    else if (parameters.input_format == "coo") {
        if(parameters.type) read_graph_coo<T,uint64_t>(graph, parameters);
        else read_graph_coo<T,uint32_t>(graph, parameters);
    }
    else if (parameters.input_format == "csr") { read_graph_csr_compressed<T>(graph, parameters); }
    else {
        std::cerr << "The input format \"" << parameters.input_format << "\" could not be found." << std::endl;
        exit(1);
    }
}

#endif // FLAGSER_INPUT_H
