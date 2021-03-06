#ifndef FLAGSER_DIRECTED_GRAPH_H // F
#define FLAGSER_DIRECTED_GRAPH_H

#include "definitions.h"


//Uncompressed Class
//Adjacency matrix is stored as a deque of 64 bit ints by taking the row of the matrix
//for vertex i considering it as a binary string, split into 64 bit sections and save
//the resulting integers
class directed_graph_t{
public:
	vertex_index_t number_of_vertices;
	bool transpose;
	std::deque<size_t> incidence_outgoing; // This is the adjacency matrix stored as a deque of 64-bit masks
	size_t incidence_row_length;

  //Constructors
	directed_graph_t(bool _transpose)
	    : number_of_vertices(0), transpose(_transpose), incidence_row_length(0) {}
	directed_graph_t(vertex_index_t _number_of_vertices, bool _transpose)
	    : number_of_vertices(_number_of_vertices), transpose(_transpose), incidence_row_length((_number_of_vertices >> 6) + 1) {
		incidence_outgoing.resize(incidence_row_length * _number_of_vertices, 0);
	}


  vertex_index_t vertex_number() const { return number_of_vertices; }

	//Used for flagser format where number of vertices is not known at time of graph construction
	virtual void set_number_of_vertices(vertex_index_t _number_of_vertices){
		if(number_of_vertices != _number_of_vertices){
                  number_of_vertices = _number_of_vertices;
                  incidence_row_length = (_number_of_vertices >> 6) + 1;
		  incidence_outgoing.resize(incidence_row_length * _number_of_vertices, 0);
	  }
	}

	virtual void add_edge(vertex_index_t v, vertex_index_t w) {
		if(v >= number_of_vertices || w >= number_of_vertices){
			std::cerr << "ERROR: Edge " << v << " " << w << " can't exist, as largest vertex id is " << number_of_vertices-1 << std::endl;
			exit(-1);
		}
		if(transpose){ std::swap(v,w); }
    const size_t ww = w >> 6;
		incidence_outgoing[v * incidence_row_length + ww] |= 1UL << ((w - (ww << 6)));
	}

	virtual bool is_connected_by_an_edge(vertex_index_t from, vertex_index_t to) {
		const auto t = to >> 6;
		return incidence_outgoing[incidence_row_length * from + t] & (1UL << (to - (t << 6)));
	}

  //returns out neighbours, a 64bit int's worth at a time
	virtual size_t get_outgoing_chunk(vertex_index_t from, size_t chunk_number) {
		return incidence_outgoing[incidence_row_length * from + chunk_number];
	}
};


//#############################################################################
//Compressed Class
//Stores the adjacency matrix as a vector of dense_hash_maps, one for each vertex
class compressed_directed_graph_t : public directed_graph_t {
public:
	std::vector<hash_map<vertex_index_t,bool>> incidence_outgoing;
	compressed_directed_graph_t(vertex_index_t _number_of_vertices, bool _transpose)
	    : directed_graph_t{ _transpose } {
    set_number_of_vertices(_number_of_vertices);
	}

	virtual void set_number_of_vertices(vertex_index_t _number_of_vertices){
		if(number_of_vertices != _number_of_vertices){
	    number_of_vertices = _number_of_vertices;
			incidence_outgoing.clear();
			for(int i=0; i < _number_of_vertices; i++){
					incidence_outgoing.push_back(hash_map<vertex_index_t,bool>());
					incidence_outgoing[i].set_empty_key(std::numeric_limits<vertex_index_t>::max());
			}
	  }
	}

	virtual void add_edge(vertex_index_t v, vertex_index_t w) {
		if(v >= number_of_vertices || w >= number_of_vertices){
			std::cerr << "ERROR: Edge " << v << " " << w << " can't exist, as largest vertex id is " << number_of_vertices-1 << std::endl;
			exit(-1);
		}
	  if(transpose){ incidence_outgoing[w][v] = true; }
    else{ incidence_outgoing[v][w] = true; }
  }

	virtual bool is_connected_by_an_edge(vertex_index_t from, vertex_index_t to) {
    return (incidence_outgoing[from].find(to) != incidence_outgoing[from].end());
	}

	//returns out neighbours
	virtual hash_map<vertex_index_t,bool>* get_outgoing_chunk(vertex_index_t from){
		return &incidence_outgoing[from];
	}
};

#endif // FLAGSER_DIRECTED_GRAPH_H
