
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <regex>
#include <getopt.h>
#include "json.hpp"

using json = nlohmann::json;

using namespace std;

struct LintData {
    std::string filepath;
    unsigned int line;
    std::string content;
    bool is_new_file = false;
};
std::vector<LintData> lint_results;

auto from_plain_txt(std::string file) {
  ifstream in(file);
  assert( in.good() );

  std::string line;
  while( getline( in, line ) ) {
    stringstream ssline(line);
    std::string filepath;
    unsigned int line;
    std::string message;

    ssline >> filepath;
    ssline >> line;
    getline( ssline, message );
    message = std::regex_replace(message, std::regex("^ +| +$|( ) +"), "$1");
    lint_results.emplace_back( LintData{filepath, line, message} );
  }
}

void print_help(){

}

// TODO implement this with a vector
auto build_json_string(std::pair<std::string,std::string> parameters[]) {
    return 0;
}

std::string exec(const char* cmd) {
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) return "ERROR";
  int buffer_size = 4096;
  char buffer[buffer_size];
  std::string result = "";
  while (!feof(pipe.get())) {
    if (fgets(buffer, buffer_size, pipe.get()) != NULL)
      result += buffer;
  }
  return result;
}

// TODO if needed parse the filenames here
auto parse_header( std::string& line ) {
  if ( line.substr( 0, 10 ) == "diff --git" ) return true; 
  return false;
}

auto parse_file_line( std::string& line ) {
  if ( line.substr(0, 3 ) == "---" || line.substr(0,3) == "+++" ) return true;
  return false;
}

auto get_line( stringstream& in, std::string error ){
  std::string line;
  if ( !getline( in, line ) ) {
    std::cout << error << " end of stream" << std::endl;
    exit(-1);
  }
  return line;
}

typedef std::pair<std::pair<int,int>,std::pair<int,int>> offset_pairs;

auto parse_offset_line( std::string& line, offset_pairs& result ) {
  stringstream line_stream(line);
  std::string ignore;
  // consume the @@ 
  line_stream >> ignore ;
  // consume the first file offsets 
  std::string first_file_offsets;
  line_stream >> first_file_offsets;
  // consume the second file offsets 
  std::string second_file_offsets;
  line_stream >> second_file_offsets;
  // ignore the rest
  
  auto calc_offset = [](auto& string_in ){
    std::string line_offset;
    std::string length_offset;
    stringstream offsets_stream(string_in);
    getline( offsets_stream, line_offset, ',' ); 
    getline( offsets_stream, length_offset );
    stringstream line_offset_stream( line_offset );
    stringstream length_offset_stream( length_offset );
    int line_offset_int ;
    int length_offset_int;
    line_offset_stream >> line_offset_int;
    length_offset_stream >> length_offset_int;
    return make_pair( line_offset_int, length_offset_int );
  };

  auto first_file_offsets_int = calc_offset( first_file_offsets );
  auto second_file_offsets_int = calc_offset( second_file_offsets );

  result = make_pair( first_file_offsets_int, second_file_offsets_int);
  return true;
}

auto parse_file( stringstream& in ) {
  auto header_line = get_line( in, "header line" );
  if ( !parse_header ( header_line ) ) {
    std::cout << "could not understand header_line " << header_line << std::endl;
    exit(-1);
  }

  auto file_a_line = get_line( in, "file a line" );
  // something elese might be here ignore it
  if ( file_a_line.substr(0, 13) == "new file mode" ) {
    file_a_line = get_line( in, "file a line" );
  }

  if ( !parse_file_line ( file_a_line ) ) {
    std::cout << "could not understand file_line " << file_a_line << std::endl;
    exit(-1);
  }

  auto file_b_line = get_line( in, "file b line" );
  if ( !parse_file_line ( file_b_line ) ) {
    std::cout << "could not understand file_line " << file_b_line << std::endl;
    exit(-1);
  }

  offset_pairs offsets;
  auto offset_line = get_line( in, "offset line");
  if ( !parse_offset_line( offset_line, offsets ) ) {
    std::cout << "could not understand offset_line " << offset_line << std::endl;
    exit(-1);
  }

  // consume the next lines that belong to this file
  std::string line;
  int ctr = 1;
  int max_lines = std::max( offsets.first.second, offsets.second.second );
  std::cout << "max_lines " << max_lines << std::endl;
  while( getline( in, line ) ){
    if ( ctr == max_lines ) {
      break;
    }
    // TODO make inclusion masks for the lines starting with a +
    ctr++;
  }

  std::cout << "offset for this file is " << offsets.first.first << "," << offsets.first.second << " :::: " << offsets.second.first << "," << offsets.second.second  << std::endl;
  auto next_line = get_line( in, "next line" );

  std::cout << "next line in file is " << next_line << std::endl;
}


// get the lines that were changed 
auto get_inclusion_mask( std::string& api_token, std::string& diffID, std::string& api_get_raw_diff ) {

  // TODO ask phabricator for a raw diff 
  std::pair<std::string,std::string> parameters[] = {
    {"api.token", api_token},
    {"diffID", diffID}
  };

  std::string json_string = api_get_raw_diff + " "s; 

  for( auto& parameter : parameters ){
    json_string += "-d "s + parameter.first + "="s + parameter.second + " "s;
  }

  std::cout << "json_string " << json_string << std::endl;

  std::string command = "curl "s + json_string;
  std::string curl_answer = exec( command.c_str() ); 
  
  std::cout << "curl_answer: " << curl_answer << std::endl;

  // transform this to json 
  json json_data = json::parse( curl_answer );  
  cout << json_data.dump(4) << endl;

  // get the result paramter 
  std::string result_text = json_data["result"];

  cout << "result: " << result_text << endl;
  // TODO parse the diff and find the lines that were added 
  stringstream result_stream( result_text );

  // TODO advance to the lines with --- and +++
  parse_file( result_stream );

    
  // TODO build a inclusion mask that can be used to filter the linter results

  return "fill me";
}

int main(int argc, char** argv){
  // command line argument parsing

  // data with defaults
  // TODO make these values a prerequisite and dont fill them with standards
  std::string diff_id = "419";
  std::string revision_id = "176";
  std::string address = "http://tesla4.physik.uni-greifswald.de:2000";
  std::string api_token = "api-7gxhhyg4acpvr2wypno5xc22j32t"; // linter-bot api token
  std::string filename = "example.txt";
  std::string filetype = "plain_txt";

  int option = 0;
  static struct option long_options[] = { 
      {"address",  required_argument, 0, 'a'},
      {"api-token",  required_argument, 0, 't'},
      {"diff-id",  required_argument, 0, 'd'},
      {"revision-id",  required_argument, 0, 'r'},
      {"filename",  required_argument, 0, 'f'},
      {"filetype",  required_argument, 0, 'x'},
      {"help",    no_argument, 0,  'h' },
      {0,         0,                 0,  0 } 
  };  

  int options_index = 0;
  // parse all switches
  while ( int opt = getopt_long(argc, argv, "a:t:d:r:f:x:h", long_options, &options_index) ){
    if ( opt == -1 ) break;
    switch(opt) { 
      case 'a':{
	address = optarg;
	break;
      }
      case 't':{
	api_token = optarg;
	break;
      }
      case 'd' :{
	diff_id = optarg;
	break;
      }
      case 'r' :{
	revision_id = optarg;
	break;
      }
      case 'f' :{
	filename = optarg;
	break;
      }
      case 'x' :{
	filetype = optarg;
	break;
      }
      case 'h' :{
	print_help();
	break;
      }
    }
  }

  std::string api_create_inline = address + "/api/differential.createinline";
  std::string api_create_comment = address + "/api/differential.createcomment";
  std::string api_get_raw_diff = address + "/api/differential.getrawdiff";

  auto inclusion_mask = get_inclusion_mask( api_token, diff_id, api_get_raw_diff );

  if ( filetype == "plain_txt" ) {
    from_plain_txt( filename );
  }else{
    std::cout << "unknown filetype" << std::endl;
    exit(-1);
  }
  
  // create the inline comments
  
  // construct json strings
  for( auto& element : lint_results ){

    std::string json_string = api_create_inline + " "s; 

    std::pair<std::string,std::string> parameters[] = {
      {"api.token", api_token},
      {"revisionID", revision_id},
      {"diffID", diff_id},
      {"filePath", element.filepath },
      {"isNewFile", to_string(element.is_new_file)},
      {"lineNumber", to_string(element.line)},
      //{"lineLength", to_string(2)},
      {"content", element.content} 
    };

    for( auto& parameter : parameters ){
      json_string += "-d "s + parameter.first + "="s + parameter.second + " "s;
    }
    

    // send them via curl
    std::string command = "curl "s + json_string;
    system( command.c_str() );

  }

  // at the end one has to post the comment with createcomment

  // create a comment that contains all inline comments
  std::string json_string = api_create_comment + " "s; 
  std::pair<std::string,std::string> parameters[] = {
      {"api.token", api_token},
      {"revision_id", revision_id},
      {"message", "\"Adding linter results\""},
      {"attach_inlines", "\"true\""}
  };

  for( auto& parameter : parameters ){
    json_string += "-d "s + parameter.first + "="s + parameter.second + " "s;
  }

  std::string command = "curl "s + json_string;
  system( command.c_str() ); 


  return 0;
}




