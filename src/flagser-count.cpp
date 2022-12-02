#define MANY_VERTICES  // uncomment when graph has more than 65k vertices

#include "../include/argparser.h"
#include "../include/definitions.h"
#include "../include/input.h"
#include "../include/directed_flag_complex.h"

template <typename T,typename F> void count_cells(T& graph, parameters_t& parameters) {
    // Aggregated counts
    std::cout << "Done" << std::endl << "Counting Cliques..." << std::endl;

    //create the complex object
    F complex(graph, parameters);

    //Load the vertices to be considered as sources
    std::vector<uint32_t> do_vertices32;
    std::vector<uint64_t> do_vertices64;

    if (parameters.vertex_todo != "all") {
        cnpy::NpyArray skip_file = cnpy::npy_load(parameters.vertex_todo);
        if(parameters.vertex_todo_type) do_vertices64 = skip_file.as_vec<uint64_t>();
        else do_vertices32 = skip_file.as_vec<uint32_t>();
    } else{
        for(int i = 0; i < graph.vertex_number(); i++){ do_vertices32.push_back(i); }
    }

    //Compute the counts
    if(parameters.vertex_todo_type) complex.template for_each_cell<uint64_t>(do_vertices64);
    else complex.template for_each_cell<uint32_t>(do_vertices32);

    std::cout << "Printing Results." << std::endl;

    //Combine counts from each thread and compute Euler Characteristic
    size_t max_dim = 0;
    for (int i = 0; i < parameters.parallel_threads; i++) {
        max_dim = max_dim < parameters.cell_counts[i].size() ? parameters.cell_counts[i].size() : max_dim;
    }
    parameters.total_cell_count.resize(max_dim, 0);
    parameters.total_max_cell_count.resize(max_dim, 0);
    for (size_t dim = 0; dim < max_dim; dim++) {
        for (int i = 0; i < parameters.parallel_threads; i++) parameters.total_cell_count[dim] += parameters.cell_counts[i].size() > dim ? parameters.cell_counts[i][dim] : 0;
        if (parameters.max_simplices) {
            for (int i = 0; i < parameters.parallel_threads; i++) parameters.total_max_cell_count[dim] += parameters.max_cell_counts[i].size() > dim ? parameters.max_cell_counts[i][dim] : 0;
        }
        parameters.euler_characteristic += pow(-1,dim)*parameters.total_cell_count[dim];
    }

    //if --containment is given output contain_count
    if (parameters.print_containment) { parameters.output_containment(); }
    if (parameters.return_simplices) { parameters.output_simplices(); }
    if (parameters.print_edge_containment) { parameters.output_edge_containment(); }

    //if --out is given redirect std::cout to print to given file
    std::fstream of;
    std::streambuf *coutbuf = std::cout.rdbuf();
    if (parameters.print_to_file) {
        of.open(parameters.output_address, std::ios::out);
        if (of.fail()) { std::cerr << "couldn't open file " << parameters.output_address << std::endl; exit(-1); }
        std::streambuf* stream_buffer_file = of.rdbuf();
        std::cout.rdbuf(stream_buffer_file);
    }

    //Print results
    std::cout << "# [euler_characteristic cell_count_dim_0 cell_count_dim_1 ...]" << std::endl << parameters.euler_characteristic;
    for (size_t dim = 0; dim < max_dim; dim++) {
        std::cout << " " << parameters.total_cell_count[dim];
    }
    std::cout << std::endl;

    if (parameters.max_simplices) {
        std::cout << std::endl << "# [max_cell_count_dim_0 max_cell_count_dim_1 ...]" << std::endl << " ";
        for (size_t dim = 0; dim < max_dim; dim++) {
            std::cout << " " << parameters.total_max_cell_count[dim];
        }
        std::cout << std::endl;
    }

    //reset std::cout
    if (parameters.print_to_file) {
        std::cout.rdbuf(coutbuf);
    }
}

int main(int argc, char** argv) {
    parameters_t parameters(argc, argv);

    if(parameters.compressed){
       if (parameters.type){
           csr_directed_graph_t<uint64_t> graph = csr_directed_graph_t<uint64_t>(parameters.number_of_vertices);
           read_directed_graph<csr_directed_graph_t<uint64_t>>(graph, parameters);
           count_cells<csr_directed_graph_t<uint64_t>,csr_directed_flag_complex_t<uint64_t>>(graph, parameters);
       } else{
           csr_directed_graph_t<uint32_t> graph = csr_directed_graph_t<uint32_t>(parameters.number_of_vertices);
           read_directed_graph<csr_directed_graph_t<uint32_t>>(graph, parameters);
           count_cells<csr_directed_graph_t<uint32_t>,csr_directed_flag_complex_t<uint32_t>>(graph, parameters);

       }
    } else{
        directed_graph_t graph = directed_graph_t(parameters.number_of_vertices, parameters.transpose, parameters.max_simplices);
        read_directed_graph<directed_graph_t>(graph, parameters);
        count_cells<directed_graph_t,directed_flag_complex_t>(graph, parameters);
    }
}
