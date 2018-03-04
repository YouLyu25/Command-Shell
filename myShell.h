#include <vector>
#include <string>
#include <map>


class MyShell {
public:
  std::string args; //arguments of one complete command
  std::vector <std::vector <std::string> > cmd; //vector of commands

public:
  //default constructor
  MyShell () {
  }
  
  //destructor
  ~MyShell () {}

  //get user commands
  int get_cmd ();

  //replace $variable with its value
  void replace_var(std::map <std::string, std::string> &var_map);
  
  //check the validity of user commands, deal with some errors
  void check_cmd ();
  
  //parse user commands, split arguments and parameters
  void parse_cmd ();

  //check the size of complete commands
  size_t cmd_size ();
  
  //execute built-in commands
  int execute_builtin (std::map <std::string, std::string> &var_map);
  
  //search for commands in paths indicated by PATH
  bool search_cmd (std::string &path, std::string &cmd);
  
  //execute commands
  void execute_cmd (std::vector <std::string> &arglist);

  //redirect input and ouput
  void redirect (std::vector <std::string> &arglist);

  //execute commands with pipe
  void pipe_cmd (size_t pipe_index); 
};







