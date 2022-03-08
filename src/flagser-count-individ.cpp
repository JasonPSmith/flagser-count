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

    std::vector<std::thread> t(parameters.parallel_threads - 1);

    std::vector<vertex_index_t> neighbours;
    complex.get_new_possible_vertex(parameters.initial_vertex, neighbours);
    std::vector<vertex_index_t> do_verts;

    if (parameters.vertex_todo != "all") {
        cnpy::NpyArray skip_file = cnpy::npy_load(parameters.vertex_todo);
        if(parameters.vertex_todo_type) do_verts = skip_file.as_vec<uint64_t>();
        else { std::cerr << "ERROR: vertices_todo must be uint64" << std::endl; exit(-1); }
    } else {
        do_verts = neighbours;
    }

    for (size_t index = 0; index < parameters.parallel_threads - 1; ++index){
        t[index] = std::thread(&directed_flag_complex_t::worker_thread_individ, complex, index, parameters.initial_vertex, std::ref(do_verts), std::ref(neighbours));
    }
    complex.worker_thread_individ(parameters.parallel_threads - 1, parameters.initial_vertex, do_verts, neighbours); // Also do work in this thread, namely the last bit
    for (size_t i = 0; i < parameters.parallel_threads - 1; ++i) t[i].join(); // Wait until all threads stopped

    std::cout << "Printing Results." << std::endl;

    //Combine counts from each thread
    size_t max_dim = 1;
    for (int i = 0; i < parameters.parallel_threads; i++) {
        max_dim = max_dim < parameters.cell_counts[i].size() ? parameters.cell_counts[i].size() : max_dim;
    }

    parameters.total_cell_count.resize(max_dim, 0);
    parameters.total_max_cell_count.resize(max_dim, 0);
    parameters.total_cell_count[0] = 1;
    parameters.total_max_cell_count[0] = 0;
    for (size_t dim = 1; dim < max_dim; dim++) {
        for (int i = 0; i < parameters.parallel_threads; i++) parameters.total_cell_count[dim] += parameters.cell_counts[i].size() > dim ? parameters.cell_counts[i][dim] : 0;
        if (parameters.max_simplices) {
            for (int i = 0; i < parameters.parallel_threads; i++) parameters.total_max_cell_count[dim] += parameters.max_cell_counts[i].size() > dim ? parameters.max_cell_counts[i][dim] : 0;
        }
    }

    //if --containment is given output contain_count
    if (parameters.print_containment) { parameters.output_containment(); }
    if (parameters.return_simplices) { parameters.output_simplices(); }

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
    std::cout << std::endl << "# Simplices with vertex " << parameters.initial_vertex << " as source" << std::endl;
    std::cout << "# [cell_count_dim_0 cell_count_dim_1 ...]" << std::endl;
    for (size_t dim = 0; dim < max_dim; dim++) {
        std::cout << parameters.total_cell_count[dim] << " ";
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

    if (!parameters.initial_vertex_input) { std::cerr << "ERROR: flagser-count-individ requires --vertex flag" << std::endl; exit(-1); }

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
