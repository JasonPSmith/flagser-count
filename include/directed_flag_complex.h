#ifndef FLAGSER_DIRECTED_FLAG_COMPLEX_H // F
#define FLAGSER_DIRECTED_FLAG_COMPLEX_H

#include "definitions.h"
#include "directed_graph.h"
#include "argparser.h"

//#############################################################################
//Uncompressed class
class directed_flag_complex_t {
public:
	directed_graph_t& graph;
	parameters_t& parameters;

	directed_flag_complex_t(directed_graph_t& _graph, parameters_t& _parameters) : graph(_graph), parameters(_parameters) {}

  //Create threads
	void for_each_cell(std::vector<cnpy_t>& do_vertices) {
		std::vector<std::thread> t(parameters.parallel_threads - 1);

		for (size_t index = 0; index < parameters.parallel_threads - 1; ++index)
			t[index] = std::thread(&directed_flag_complex_t::worker_thread, this, index, std::ref(do_vertices));

		worker_thread(parameters.parallel_threads - 1, do_vertices); // Also do work in this thread, namely the last bit

		for (size_t i = 0; i < parameters.parallel_threads - 1; ++i) t[i].join(); // Wait until all threads stopped
	}

  //Assign to each thread the vertices to be considered as source and start the thread computing
	void worker_thread(int thread_id, std::vector<cnpy_t>& do_vertices) {

		const size_t vertices_per_thread = graph.vertex_number() / parameters.parallel_threads;

		std::vector<vertex_index_t> first_position_vertices;
		for (size_t index = thread_id; index < do_vertices.size(); index += parameters.parallel_threads)
			first_position_vertices.push_back(do_vertices[index]);

		std::vector<vertex_index_t> prefix;

		do_for_each_cell(first_position_vertices, prefix, 0, thread_id, do_vertices.size());
	}

  //iterator through cliques finding common neighbours to all current members and adding to clique
	void do_for_each_cell(const std::vector<vertex_index_t>& possible_next_vertices, std::vector<vertex_index_t>& prefix,
	                      unsigned short prefix_size, int thread_id, size_t number_of_vertices) {
		//Add this simplex to the count
		if (prefix_size > 0) { parameters.increase_count(prefix_size, thread_id); }

    // If we require all simplices printed, then print this simplex now
    if (parameters.print_simplices) output_simplices(prefix_size, prefix, thread_id);
		if (parameters.print_binary) output_binary(prefix_size, prefix, thread_id);
		if (parameters.print_containment) update_containment(prefix_size, prefix, thread_id);

		// If this is the last dimension we are interested in, exit this branch
		if (prefix_size == parameters.max_dimension + 1) return;

    for (auto vertex : possible_next_vertices) {
			// We can write the cell given by taking the current vertex as the maximal element
			std::vector<vertex_index_t> current_prefix(prefix);
			current_prefix.push_back(vertex);

			// And compute the next elements
			std::vector<vertex_index_t> new_possible_vertices;
			if (prefix_size > 0) {
				for (auto v : possible_next_vertices) {
					if (vertex != v && graph.is_connected_by_an_edge(vertex, v)) {
						new_possible_vertices.push_back(v);
					}
				}
			} else {
				get_new_possible_vertex(vertex, new_possible_vertices);
			}

      // Repeat for new possible vertices
      do_for_each_cell(new_possible_vertices, current_prefix, prefix_size + 1, thread_id, number_of_vertices);

			// Print current status of completed vertices in this thread
      if (parameters.progress) { print_status(prefix_size, vertex, thread_id, number_of_vertices); }
		}
	}

  //Get neighbours of vertex and add them to new_possible_vertices
	virtual void get_new_possible_vertex(vertex_index_t vertex, std::vector<vertex_index_t>& new_possible_vertices) {
		for (size_t offset = 0; offset < graph.incidence_row_length; offset++) {
			size_t bits = graph.get_outgoing_chunk(vertex, offset);

			size_t vertex_offset = offset << 6;
			while (bits > 0) {
				int b = __builtin_ctzl(bits);  // Get the least significant non-zero bit
				bits &= ~(1UL << b);           // Unset this bit
				new_possible_vertices.push_back(vertex_offset + b);
			}
		}
	}

  //print current count in each thread
  void print_status(unsigned short prefix_size, vertex_index_t vertex, int thread_id, size_t number_of_vertices){
    if (prefix_size == 0){
      {
          LockIO lock;
          parameters.vertices_completed++;
          std::cout << vertex << " : thread " << thread_id << " : ";
          for( int i=0; i < parameters.cell_counts[thread_id].size(); i++) std::cout << parameters.cell_counts[thread_id][i] << " ";
          std::cout <<" : " << parameters.vertices_completed << "/" << number_of_vertices << std::endl;
      }
    }
  }

	//Used when --containment flag is inputted
	void update_containment(unsigned short prefix_size, std::vector<vertex_index_t>& prefix, int thread_id){
	  for(int i = 0; i < prefix_size; i++){
			//Ensure contain_counts goes up to current dimension and then increase that dimension by 1
		  while(parameters.contain_counts[thread_id][prefix[i]].size() < prefix_size){
				parameters.contain_counts[thread_id][prefix[i]].push_back(0);
		  }
		  parameters.contain_counts[thread_id][prefix[i]][prefix_size-1]++;
    }
	}

  //Used when --binary flag is inputted
  void output_binary(unsigned short prefix_size, std::vector<vertex_index_t>& prefix, int thread_id){
		if(prefix_size > parameters.min_print && prefix_size <= parameters.max_print+1){

      std::vector<std::bitset<64>> bits(ceil((double)prefix_size/3));  //Create a vector of 64 bit ints stored as bitsets
			for(int i = 0; i < bits.size(); i++){ bits[i].flip(); } //Set all bits to 1
			bits[0][63] = 0; //Set leading bit to 0

			//Consider each vertex in clique
			for(int i = 0; i < prefix_size; i++){
					std::bitset<64> thisbit(prefix[i]);  //Get vertex as bitset
					int offset = i%3;                    //Workout which 21 bit int in the 64bit
					for(int j = 0; j < 21; j++){
							bits[i/3][j+offset*21] = thisbit[j];
					}
			}

			// Print the 64bit ints to the binary files
			for(int i = 0; i < bits.size(); i++){
					parameters.binary_outstreams[thread_id].write((char*)(&bits[i]), sizeof(bits[i]));
			}
	  }
  }

	//Used when --print flag is inputted
  void output_simplices(unsigned short prefix_size, std::vector<vertex_index_t>& prefix, int thread_id){
    if(prefix_size > 1 && prefix_size > parameters.min_print && prefix_size <= parameters.max_print+1) {
		  for(int i = 0; i < prefix_size; i++){ parameters.simplices_outstreams[thread_id] << prefix[i] << " "; }
		  parameters.simplices_outstreams[thread_id] << std::endl;
    }
	}
};


//#############################################################################
//Compressed class
class compressed_directed_flag_complex_t : public directed_flag_complex_t {
public:
	compressed_directed_graph_t& graph;
	compressed_directed_flag_complex_t(compressed_directed_graph_t& _graph, parameters_t& _parameters) : directed_flag_complex_t{ _graph, _parameters }, graph(_graph) {}

  //Get neighbours of vertex and add them to new_possible_vertices
	virtual void get_new_possible_vertex(vertex_index_t vertex, std::vector<vertex_index_t>& new_possible_vertices) {
    hash_map<vertex_index_t,bool>* out_neigh = graph.get_outgoing_chunk(vertex);
    for(auto iter = out_neigh->begin(); iter != out_neigh->end(); ++iter){
		  new_possible_vertices.push_back(iter->first);
		}
	}
};

#endif // FLAGSER_DIRECTED_FLAG_COMPLEX_H
