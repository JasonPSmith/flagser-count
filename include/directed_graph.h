#ifndef FLAGSER_DIRECTED_GRAPH_H // F
#define FLAGSER_DIRECTED_GRAPH_H

#include "definitions.h"
#include "cnpy.h"


//Uncompressed Class
//Adjacency matrix is stored as a deque of 64 bit ints by taking the row of the matrix
//for vertex i considering it as a binary string, split into 64 bit sections and save
//the resulting integers
class directed_graph_t{
public:
    vertex_index_t number_of_vertices;
    bool transpose;
    bool store_incoming;
    std::deque<size_t> incidence_outgoing; // This is the adjacency matrix stored as a deque of 64-bit masks
    std::deque<size_t> incidence_incoming;
    size_t incidence_row_length;

    //Constructors
    directed_graph_t(bool _transpose, bool _max_simplices)
        : number_of_vertices(0), transpose(_transpose), store_incoming(_max_simplices), incidence_row_length(0) {}
    directed_graph_t(vertex_index_t _number_of_vertices, bool _transpose, bool _max_simplices)
        : number_of_vertices(_number_of_vertices), transpose(_transpose), store_incoming(_max_simplices), incidence_row_length((_number_of_vertices >> 6) + 1) {
        incidence_outgoing.resize(incidence_row_length * _number_of_vertices, 0);
        if (store_incoming){ incidence_incoming.resize(incidence_row_length * _number_of_vertices, 0); }
    }


    vertex_index_t vertex_number() const { return number_of_vertices; }

    //Used for flagser format where number of vertices is not known at time of graph construction
    virtual void set_number_of_vertices(vertex_index_t _number_of_vertices){
        if(number_of_vertices != _number_of_vertices){
            number_of_vertices = _number_of_vertices;
            incidence_row_length = (_number_of_vertices >> 6) + 1;
            incidence_outgoing.resize(incidence_row_length * _number_of_vertices, 0);
            if (store_incoming){ incidence_incoming.resize(incidence_row_length * _number_of_vertices, 0); }
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
        if (store_incoming){
            const size_t vv = v >> 6;
            incidence_incoming[w * incidence_row_length + vv] |= 1UL << ((v - (vv << 6)));
        }
    }

    virtual bool is_connected_by_an_edge(vertex_index_t from, vertex_index_t to) {
        const auto t = to >> 6;
        return incidence_outgoing[incidence_row_length * from + t] & (1UL << (to - (t << 6)));
    }

    //returns out neighbours, a 64bit int's worth at a time
    virtual size_t get_outgoing_chunk(vertex_index_t from, size_t chunk_number) {
        return incidence_outgoing[incidence_row_length * from + chunk_number];
    }
    virtual size_t get_incoming_chunk(vertex_index_t from, size_t chunk_number) {
        return incidence_incoming[incidence_row_length * from + chunk_number];
    }
    virtual void add_edges(cnpy::NpyArray indices_file, cnpy::NpyArray indptr_file){}
};


//#############################################################################
//Compressed Class
//Stores the adjacency matrix as a vector of dense_hash_maps, one for each vertex
class compressed_directed_graph_t : public directed_graph_t {
public:
    std::vector<hash_map<vertex_index_t,bool>> incidence_outgoing;
    std::vector<hash_map<vertex_index_t,bool>> incidence_incoming;
    compressed_directed_graph_t(vertex_index_t _number_of_vertices, bool _transpose, bool _max_simplices)
      : directed_graph_t{ _transpose, _max_simplices } {
        set_number_of_vertices(_number_of_vertices);
    }

    virtual void set_number_of_vertices(vertex_index_t _number_of_vertices){
        if(number_of_vertices != _number_of_vertices){
            number_of_vertices = _number_of_vertices;
            incidence_outgoing.clear();
            if (store_incoming){ incidence_incoming.clear(); }
            for(int i=0; i < _number_of_vertices; i++){
                incidence_outgoing.push_back(hash_map<vertex_index_t,bool>());
                incidence_outgoing[i].set_empty_key(std::numeric_limits<vertex_index_t>::max());
                if (store_incoming){
                    incidence_incoming.push_back(hash_map<vertex_index_t,bool>());
                    incidence_incoming[i].set_empty_key(std::numeric_limits<vertex_index_t>::max());
                }
            }
        }
    }

    virtual void add_edges(cnpy::NpyArray indices_file, cnpy::NpyArray indptr_file){}

    virtual void add_edge(vertex_index_t v, vertex_index_t w) {
        if(v >= number_of_vertices || w >= number_of_vertices){
            std::cerr << "ERROR: Edge " << v << " " << w << " can't exist, as largest vertex id is " << number_of_vertices-1 << std::endl;
            exit(-1);
        }
        if(transpose){ std::swap(v,w); }
        incidence_outgoing[v][w] = true;
        if (store_incoming){ incidence_incoming[w][v] = true; }
    }

    virtual bool is_connected_by_an_edge(vertex_index_t from, vertex_index_t to) {
        return (incidence_outgoing[from].find(to) != incidence_outgoing[from].end());
    }

    //returns out neighbours
    virtual hash_map<vertex_index_t,bool>* get_outgoing_chunk(vertex_index_t from){
        return &incidence_outgoing[from];
    }
    virtual hash_map<vertex_index_t,bool>* get_incoming_chunk(vertex_index_t from){
        return &incidence_incoming[from];
    }
};

//#############################################################################
//CSR Class
//Stores the adjacency matrix in compressed sparse row format
class csr_directed_graph_t : public directed_graph_t {
public:
    std::vector<cnpy_t> indices;
    std::vector<cnpy_t> indptr;
    csr_directed_graph_t(vertex_index_t _number_of_vertices)
      : directed_graph_t{ _number_of_vertices, false, false } {
    }

    virtual void add_edges(cnpy::NpyArray indices_file, cnpy::NpyArray indptr_file){
        indices = indices_file.as_vec<cnpy_t>();
        indptr = indptr_file.as_vec<cnpy_t>();
    }

    virtual bool is_connected_by_an_edge(vertex_index_t from, vertex_index_t to) {
        //return std::find(indices.begin()+indptr[from],indices.begin()+indptr[from+1],to) != indices.begin()+indptr[from+1];
        for(int i = indptr[from]; i < indptr[from+1]; i++){
            if(indices[i] == to){ return true; }
        }
        return false;
    }

    virtual cnpy_t get_outgoing_start(vertex_index_t from){
        return indptr[from];
    }
    virtual cnpy_t get_outgoing_end(vertex_index_t from){
        return indptr[from+1];
    }
};

#endif // FLAGSER_DIRECTED_GRAPH_H
