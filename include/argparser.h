#ifndef FLAGSER_ARGPARSER_H
#define FLAGSER_ARGPARSER_H

#include "definitions.h"

typedef std::unordered_map<std::string, const char*> named_arguments_t;

//Parse the inputted argument by creating pairs (arg_name, value)
const named_arguments_t parse_arguments(int argc, char** argv) {
	named_arguments_t named_arguments;

	for (size_t i = 1; i < argc; ++i) {
		std::string arg(argv[i]);
		arg = arg.substr(2); // remove --

		// These flags have no additional data attached
		if (arg == "transpose" || arg == "compressed" || arg == "progress") {
			named_arguments.insert(std::make_pair(arg, "true"));
			continue;
		}

		named_arguments.insert(std::make_pair(arg, argv[++i]));  // We have extra data, so parse it
	}

	return named_arguments;
}

//A structure that contains all the inputted data, and stores the results
struct parameters_t {
  unsigned short min_dimension = 0;
	unsigned short max_dimension = std::numeric_limits<unsigned short>::max();
	unsigned short parallel_threads = 8;
	vertex_index_t number_of_vertices = 0;
	unsigned short min_print = 2;                                          //inclusive
	unsigned short max_print = std::numeric_limits<unsigned short>::max(); //inclusive
	size_t vertices_completed = 0;
	bool print_simplices = false;
	bool print_binary = false;
	bool print_containment = false;
	bool print_to_file = false;
	bool progress;
	bool compressed;
	bool transpose;

  //input output elements
	std::ofstream containment_outstream;
	std::vector<std::ofstream> simplices_outstreams;
	std::vector<std::fstream> binary_outstreams;
	std::vector<std::vector<std::vector<vertex_index_t>>> contain_counts;
	std::vector<std::vector<vertex_index_t>> cell_counts;
	std::vector<vertex_index_t> do_vertices;
	std::string input_format = "flagser";
	std::string input_address1;
	std::string input_address2;
	std::string vertex_todo;
	std::string output_address;

  //Constructor
	parameters_t(int argc, char** argv){
		named_arguments_t named_arguments = parse_arguments(argc, argv);

		//iterators to find inputted arguments
		auto it_todo = named_arguments.find("vertices-todo");
		auto it_out = named_arguments.find("out");
		auto it_threads = named_arguments.find("threads");
		auto it_size = named_arguments.find("size");
		auto it_format = named_arguments.find("in-format");
		auto it_max = named_arguments.find("max_dim");
		auto it_binary = named_arguments.find("binary");
		auto it_print = named_arguments.find("print");
		auto it_contain = named_arguments.find("containment");
		auto it_max_print = named_arguments.find("max_print");
		auto it_min_print = named_arguments.find("min_print");
    named_arguments_t::const_iterator it;

    //Arguments with no additional data are given true if they are inputted, otherwise false
    compressed = ((it = named_arguments.find("compressed")) != named_arguments.end());
		transpose = ((it = named_arguments.find("transpose")) != named_arguments.end());
		progress = ((it = named_arguments.find("progress")) != named_arguments.end());


    //integer arguments
		if (it_max != named_arguments.end()) { max_dimension = atoi(it_max->second); }
		if (it_threads != named_arguments.end()) { parallel_threads = atoi(it_threads->second); }
		if (it_max_print != named_arguments.end()) { max_print= atoi(it_max_print->second); }
		if (it_min_print != named_arguments.end()) { min_print= atoi(it_min_print->second); }

    //input format arguments
    if (it_format != named_arguments.end()) { input_format = it_format->second; }
		if (input_format == "csr") { input_format = "csc"; transpose = true; }
		if (it_size != named_arguments.end()) {
			number_of_vertices = atoi(it_size->second);
		} else if (input_format == "coo" || input_format == "csc" || input_format == "edge-list" ){
      std::cerr << "ERROR: Number of vertices n must be inputed with \"--size n\" for formats edge-list, csc and coo." << std::endl;
			exit(-1);
		}

		if (input_format == "edge-list") { input_format = "flagser"; }
    if (input_format == "flagser"){
      auto it_file = named_arguments.find("in");
			if (it_file == named_arguments.end()) {
				std::cerr << "ERROR: File address must be given for flagser format" << std::endl; exit(-1);
			}
			input_address1 = it_file->second;
		} else if(input_format == "csc") {
			auto it_indices = named_arguments.find("indices");
			auto it_indptr = named_arguments.find("indptr");
			if (it_indices == named_arguments.end() || it_indptr == named_arguments.end()) {
				std::cerr << "ERROR: indices and indptr files are required for csc format" << std::endl; exit(-1);
			}
      input_address1 = it_indices->second;
			input_address2 = it_indptr->second;
	  } else if(input_format == "coo") {
			auto it_row = named_arguments.find("row");
			auto it_col = named_arguments.find("col");
			if (it_row == named_arguments.end() || it_col == named_arguments.end()) {
				std::cerr << "ERROR: row and column files are required for coo format" << std::endl; exit(-1);
			}
      input_address1 = it_row->second;
			input_address2 = it_col->second;
		}


    //If --vertices-todo is included save the file address otherwise set so all vertices are considered
		if (it_todo != named_arguments.end()) { vertex_todo = it_todo->second; }
		else { vertex_todo = "all";	}

    //if --out is included save the address of the outfile and save that we are not printing to terminal
		if (it_out != named_arguments.end()) {
			output_address = it_out->second;
			print_to_file = true;
			std::ifstream f(output_address);
			if (f.good()) { std::cerr << "The output file already exists, aborting." << std::endl; exit(-1); }
		}

    //If --binary is included, set boolean and create binary outstreams, one for each thread.
		if (it_binary != named_arguments.end()) {
			print_binary = true;
		  for (int i = 0; i < parallel_threads; i++){
				binary_outstreams.push_back(std::fstream(it_binary->second+std::to_string(i)+".binary", std::ios::out | std::ios::binary));
		  }
		}

    //If --print is included, set boolean and create outstreams, one for each thread.
		if (it_print != named_arguments.end()) {
			print_simplices = true;
		  for (int i = 0; i < parallel_threads; i++){
				simplices_outstreams.push_back(std::ofstream(it_print->second+std::to_string(i)+".simplices"));
		  }
		}

    //If --containment, set boolean and create vectors to store the counts, one for each thread and each vertex
		if (it_contain != named_arguments.end()) {
			if (it_size == named_arguments.end()) {
				std::cerr << "ERROR: Number of vertices n must be inputed with \"--size n\" when using --containment" << std::endl;exit(-1);
			}
      contain_counts.assign(parallel_threads, std::vector<std::vector<vertex_index_t>>(number_of_vertices, std::vector<vertex_index_t>(0)));
			print_containment = true;
			containment_outstream = std::ofstream(it_contain->second);
		}
		cell_counts.assign(parallel_threads, std::vector<vertex_index_t>(0));
	} //end constructor



	// function for printing containment values
	void output_containment(){
		//combine results from different threads and put in contain_counts[0]
		for(int i = 1; i < contain_counts.size(); i++){
        for(int j = 0; j < contain_counts[i].size(); j++){
            while(contain_counts[0][j].size() < contain_counts[i][j].size()){
                contain_counts[0][j].push_back(0);
            }
            for(int k = 0; k < contain_counts[i][j].size(); k++){
                contain_counts[0][j][k] += contain_counts[i][j][k];
            }
        }
    }
		//print containment values
    for(int i = 0; i < contain_counts[0].size(); i++){
        for(int j = 0; j < contain_counts[0][i].size(); j++){
            containment_outstream << contain_counts[0][i][j] << " ";
        }
        containment_outstream << std::endl;
    }
	}
	//increase count of containment values
	void increase_count(unsigned short size, int thread_id){
		if (cell_counts[thread_id].size() < size) { cell_counts[thread_id].resize(size, 0); }
		cell_counts[thread_id][size - 1]++;
	}
};

#endif // FLAGSER_ARGPARSER_H
