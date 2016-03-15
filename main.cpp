
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <regex>
#include <getopt.h>

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
      {"adress",  required_argument, 0, 'a'},
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
    
    std::cout << "json_string " << json_string << std::endl;

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




