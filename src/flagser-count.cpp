#define MANY_VERTICES  // uncomment when graph has more than 65k vertices

#include "../include/argparser.h"
#include "../include/definitions.h"
#include "../include/input.h"
#include "../include/directed_flag_complex.h"

template <typename T,typename F> void count_cells(T& graph, parameters_t& parameters) {
	// Aggregated counts
	std::vector<size_t> total_cell_count;
	index_t total_euler_characteristic = 0;
	size_t total_max_dim = 0;

	std::cout << "Done" << std::endl << "Counting Cliques..." << std::endl;

  //create the complex object
	F complex(graph, parameters);

	//Load the vertices to be considered as sources
	std::vector<vertex_index_t> do_vertices;
	if (parameters.vertex_todo != "all") {
			cnpy::NpyArray skip_file = cnpy::npy_load(parameters.vertex_todo);
			do_vertices = skip_file.as_vec<vertex_index_t>();
	} else{
			for(int i = 0; i < graph.vertex_number(); i++){ do_vertices.push_back(i); }
	}

  //Compute the counts
	complex.for_each_cell(do_vertices);

  std::cout << "Printing Results." << std::endl;

  //Combine counts from each thread and compute Euler Characteristic
	int64_t euler_characteristic = 0;
	size_t max_dim = 0;
	for (int i = 0; i < parameters.parallel_threads; i++) {
		max_dim = max_dim < parameters.cell_counts[i].size() ? parameters.cell_counts[i].size() : max_dim;
	}
	total_cell_count.resize(max_dim, 0);
	for (size_t dim = 0; dim < max_dim; dim++) {
		for (int i = 0; i < parameters.parallel_threads; i++) total_cell_count[dim] += parameters.cell_counts[i].size() > dim ? parameters.cell_counts[i][dim] : 0;
		euler_characteristic += pow(-1,dim)*total_cell_count[dim];
	}

  //if --containment is given output contain_count
  if (parameters.print_containment) { parameters.output_containment(); }

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
	std::cout << "# [euler_characteristic cell_count_dim_0 cell_count_dim_1 ...]" << std::endl << euler_characteristic;
	for (size_t dim = 0; dim < max_dim; dim++) {
		std::cout << " " << total_cell_count[dim];
	}
	std::cout << std::endl;

  //reset std::cout
	if (parameters.print_to_file) {
    std::cout.rdbuf(coutbuf);
	}
}

int main(int argc, char** argv) {
	parameters_t parameters(argc, argv);

  if(parameters.compressed){
		 compressed_directed_graph_t graph = compressed_directed_graph_t(parameters.number_of_vertices, parameters.transpose);
     read_directed_graph<compressed_directed_graph_t>(graph, parameters);
     count_cells<compressed_directed_graph_t,compressed_directed_flag_complex_t>(graph, parameters);
  } else{
     directed_graph_t graph = directed_graph_t(parameters.number_of_vertices, parameters.transpose);
		 read_directed_graph<directed_graph_t>(graph, parameters);
     count_cells<directed_graph_t,directed_flag_complex_t>(graph, parameters);
  }
}
