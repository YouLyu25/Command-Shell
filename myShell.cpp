#include "myShell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <dirent.h>
#include <map>

#define MAX_SIZE 512
#define MAX_ARGS 256

#define DEBUG_PARSE_CMD 0

//global variable
std::map <std::string, std::string> variable_map; //map for storing user-set variables


/* get a line of commands from stdin and perform checks for "exit", EOF and
   empty commands */
int MyShell::get_cmd () {
  char curr_path[MAX_SIZE];
  char *temp_path = getcwd(curr_path, sizeof(curr_path));
  temp_path = temp_path;
  
  //get command line
  std::cout << "myShell:" << curr_path << " $ ";
  std::getline(std::cin, args);
  
  //exit when command is EOF or "exit" is read from stdin
  if (std::cin.eof()) {
    std::cout << "\nProgram exited with status " << EXIT_SUCCESS << "\n";
    //clear input buffer to prevent duplication of reading by child process
    //this is used when we redirect our input, i.e. ./myShell < input.txt
    setbuf(stdin, NULL);
    exit(EXIT_SUCCESS);
  }
  
  if (args == "exit") {
    std::cout << "Program exited with status " << EXIT_SUCCESS << "\n";
    setbuf(stdin, NULL);
    exit(EXIT_SUCCESS);
  }
  //return -1 when no command is entered
  else if (args.size() == 0) {
    setbuf(stdin, NULL);
    return -1;
  }

  setbuf(stdin, NULL);
  return 0;
}



/* the function replaces $var_name with its value, e.g. if abc is a variable in
   current environment which has a value of cd, then $var_name in the commands
   will be replaced by cd; we perform this replacement before further parse the
   commands */
void MyShell::replace_var (std::map <std::string, std::string> &var_map) {
    //if empty command
    if (args.size() == 0) {
      exit(EXIT_SUCCESS);
    }

    //take "abc$def$gh:ij" as an instance
    while (1) {
      //search one complete command for '$'
      std::size_t found = args.find("$");
      if (found != std::string::npos) {
        //store substring after '$' into a new string substr1 ("def$gh:ij")
        std::string substr1 = args.substr(found+1);
        std::string substr2;
        
        for (size_t j = 0; j < substr1.size(); ++j) {
          //check if substr1 have invalid character for variable name
          if (!((substr1[j]>47 && substr1[j]<58) || (substr1[j]>64 && substr1[j]<91) || (substr1[j]>96 && substr1[j]<123) || (substr1[j]==95))) {
            std::size_t substr_found = substr1.find(substr1[j]);
            //substr2 is "$gh:ij"
            substr2 = substr1.substr(substr_found);
            //remove extra content from substr1, substr1 is now "def"
            substr1.erase(substr_found, std::string::npos);
            break;
          }
        }
        //substr1 is now the variable name to be replaced
        std::string var_to_replace = substr1;
        args.erase(found, std::string::npos);
        
        //check var_map and get var_value for replacing
        if (var_map.find(var_to_replace) != var_map.end()) {
          std::string var_value = var_map.find(var_to_replace)->second;
          var_to_replace = var_value;
          //add replaced value into cmd
          args.append(var_to_replace);
          args.append(substr2);
        }
        else {
          //if no match in var_map, check environ
          if ((getenv(var_to_replace.c_str())) != NULL) {
            //replace variable name with its value
            char *var_value = getenv(var_to_replace.c_str());
            var_to_replace = var_value;
            //add replaced value into cmd
            args.append(var_to_replace);
            args.append(substr2);
          }
          else {
            //unknown variable, ignore this command
            std::cerr << "unknown variable name " << var_to_replace << "\n";
            args.clear();
          }
        }
      }
      //all '$' evaliation have been performed
      else {
        break;
      }
    }//end while(1)
}



/* the function checks the validity of commands, specifically: exits when the
   first argument is "exit"; checks the syntax near "<", ">" and "2>"; checks
   if the number of arguments and the total size of commands exceed limits
   defined as MAX_ARGS and MAX_SIZE */
void MyShell::check_cmd () {
  int cmd_size = 0;
  int arg_size = 0;

  if (cmd.size() > 0) {
    //if the first argument is "exit", then exit the program
    if (cmd[0][0] == "exit") {
      std::cout << "Program exited with status " << EXIT_SUCCESS << "\n";
      exit(EXIT_SUCCESS);
    }
    
    //no ">", "<" or "2>" should follow either ">", "<" or "2>"
    for (size_t i = 0; i < cmd.size(); ++i) {
      //check total number of arguments for all commands
      arg_size += cmd[i].size();

      for (size_t j = 0; j < cmd[i].size(); ++j) {
        //check the overall size of commands entered
        cmd_size += cmd[i][j].size();
        
        //check syntax near ">"
        if (cmd[i][j] == ">" && cmd[0][0] != "set") {
          if (j+1 < cmd[i].size()) {
            if (cmd[i][j+1] == ">" || cmd[i][j+1] == "<" || cmd[i][j+1] == "2>") {
              std::cerr << "syntax error after '>'\n";
              //treat this line of commands as empty commands
              cmd.clear();
              break;
            }
          }
          //report error when no filename after ">"
          else if (j+1 == cmd[i].size()) {
            std::cerr << "syntax error with '>'\n";
            //treat this line of commands as empty commands
            cmd.clear();
            break;
          }
        }
        //check syntax near "<"
        else if (cmd[i][j] == "<" && cmd[0][0] != "set") {
          if (j+1 < cmd[i].size()) { 
            if (cmd[i][j+1] == ">" || cmd[i][j+1] == "<" || cmd[i][j+1] == "2>") {
              std::cerr << "syntax error after '<'\n";
              //treat this line of commands as empty commands
              cmd.clear();
              break;
            }
          }
          //report error when no filename after "<"
          else if (j+1 == cmd[i].size()) {
            std::cerr << "syntax error with '<'\n";
            //treat this line of commands as empty commands
            cmd.clear();
            break;
          }
        }
        //check syntax near "2>"
        else if (cmd[i][j] == "2>" && cmd[0][0] != "set") {
          if (j+1 < cmd[i].size()) { 
            if (cmd[i][j+1] == ">" || cmd[i][j+1] == "<" || cmd[i][j+1] == "2>") {
              std::cerr << "syntax error after '2>'\n";
              //treat this line of commands as empty commands
              cmd.clear();
              break;
            }
          }
          //report error when no filename after "2>"
          else if (j+1 == cmd[i].size()) {
            std::cerr << "syntax error with '2>'\n";
            //treat this line of commands as empty commands
            cmd.clear();
            break;
          }
        }
      }
    }
    //if size of arguments or commands exceeds the limit, treat this line of commands as empty commands
    if (arg_size > MAX_ARGS) {
      std::cerr << "arguments number exceeds limit " << MAX_ARGS << "\n";
      cmd.clear();
    }
    else if (cmd_size > MAX_SIZE) {
      std::cerr << "overall size of commands exceeds limit " << MAX_SIZE << "\n";
      cmd.clear();
    }
  }
}



/* the function parses commands read from files or stdin, divides arguments with
   white space and divides commands with '|', specifically handles white space
   escaped with '\' as well as "<", ">" and "2>" */
void MyShell::parse_cmd () {
  std::vector <std::string> arglist;
  std::string one_arg;
  size_t i = 0;
  
  replace_var(variable_map);

  for (i = 0; i < args.size(); ++i) {
    if (args[i] == ' ') {
      //handle white space escaped with '\'
      if (args[i-1] == '\\') {
        //erase '\' added previously (the last element in one_arg)
        one_arg.erase(one_arg.end()-1);
        //add space to the argument
        one_arg.push_back(args[i]);
      }
      else {
        if (one_arg.size() > 0) {
          //add one complete argument to the command argument list
          arglist.push_back(one_arg);
          //clear temp string
          one_arg.clear();
        }
      }
    }
    //parse commands with '>', '<' or "2>"
    //use these to separate commands, i.e. a>b and a > b are both valid
    else if (args[i] == '>') {
      //if '>' is followed by a '2'
      if (args[i-1] == '2') {
        if ((i == 1) || (one_arg == "2")) {
          //treat "2>" as one argument
          one_arg.push_back(args[i]);
          arglist.push_back(one_arg);
          one_arg.clear();
        }
        else {
          //add to one of the args and arglist
          arglist.push_back(one_arg);
          one_arg.clear();
          one_arg.push_back(args[i]);
          arglist.push_back(one_arg);
          one_arg.clear();
        }
      }
      //if '>' is not followed by a '2'
      else {
        if (i != 0) {
          if (one_arg.size() > 0) {
            arglist.push_back(one_arg);
            one_arg.clear();
          }
        }
        one_arg.push_back(args[i]);
        arglist.push_back(one_arg);
        one_arg.clear();
      }
    }
    else if (args[i] == '<') {
      if (i != 0) {
        if (one_arg.size() > 0) {
          arglist.push_back(one_arg);
          one_arg.clear();
        }
      }
      //add to one of the args and arglist
      one_arg.push_back(args[i]);
      arglist.push_back(one_arg);
      one_arg.clear();
    }

    //check '|' for pipe, split commands
    else if (args[i] == '|') {
      //'|' should not be the first or the last command
      if (i == 0) {
        std::cerr << "syntax error with '|'\n";
        cmd.clear();
        one_arg.clear();
        arglist.clear();
        break;
      }
      else if (i+1 == args.size()) {
        //note that if set, there is different logic
        if (arglist[0] != "set") {
          std::cerr << "syntax error with '|'\n";
          cmd.clear();
          one_arg.clear();
          arglist.clear();
        }
        break;
      }
      //case for "abc | abc"
      if (one_arg.size() == 0 && arglist.size() > 0) {
          cmd.push_back(arglist);
          arglist.clear();
      }
      //case for "abc|abc"
      else {
          arglist.push_back(one_arg);
          one_arg.clear();
          cmd.push_back(arglist);
          arglist.clear();
      }
    }
    else {
      //add one element to an argument
      one_arg.push_back(args[i]);
    }
  }

  //don't forget the last argument without ' ' at the end
  if (one_arg.size() > 0) {
    arglist.push_back(one_arg);
  }
  
  //and last command group without '|'
  if (arglist.size() > 0) {
    cmd.push_back(arglist);
  }

  //check the validity of commands (">", "<" and "2>")
  check_cmd();
  
  //print out the parsed commands for debugging 
#if DEBUG_PARSE_CMD
  std::cout << "commands {";
  for (size_t i = 0; i < cmd.size(); ++i) {
    if (i+1 < cmd[i].size()) {
      std::cout << "\n";
    }
    for (size_t j = 0; j < cmd[i].size(); ++j) {
      std::cout << cmd[i][j] << ":";
    }
  }
  std::cout << "\n}\n";
#endif
}



/* get the number of commands divided by "|", if no pipe, then size will be 1 */
size_t MyShell::cmd_size () {
  return cmd.size();
}



/* execute built-in commands including "pwd", "cd", "$" evaluiation, "set" and
   "export" before execute any binary program, if the command is not one of
   the built-in commands than it will be sent forward to pipe_cmd() */
int MyShell::execute_builtin (std::map <std::string, std::string> &var_map) {
  //if no argument, do nothing
  if (cmd[0].size() == 0) {
    return -1;
  }
  
  //built-in commands "pwd" and "cd"
  if (cmd[0][0] == "pwd") {
    char curr_path[MAX_SIZE];
    char *temp_path = getcwd(curr_path, sizeof(curr_path));
    temp_path = temp_path;
    std::cout << curr_path << "\n";
    return 1;
  }
  else if (cmd[0][0] == "cd") {
    //if enter "cd" without other argument or with "~", change directory to home path
    if (cmd[0].size() == 1 || cmd[0][1] == "~") {
      char *args = getenv("HOME");
      if (chdir(args) >= 0) {
        return 1;
      }
      else {
        perror("-bash: cd");
        return -1;
      }
    }
    //white space is not a valid directory
    else if (cmd[0][1] == " ") {
      perror("-bash: cd");
      return -1;
    }
    else {
      //change directory using chdir()
      if (chdir(cmd[0][1].c_str()) >= 0) {
        return 1;
      }
      else {
        perror("-bash: cd");
        return -1;
      }
    }
  }
  //built-in commands
  //set var value (store var and its value in a map)
  else if (cmd[0][0] == "set") {
    std::string var_name;
    
    //re-process strings after set and var_name
    if (cmd[0].size() > 2) {
      //std::size_t pos_found = args.find(cmd[0][2]);
      if (cmd[0].size() > 3) {
        std::size_t pos_found1 = cmd[0][0].size() + cmd[0][1].size() + 1;
        std::size_t pos_found2 = args.find(cmd[0][2]);
        std::string temp_substr;
        while (1) {
          if (pos_found2 >= pos_found1) {
            temp_substr = args.substr(pos_found2);
            break;
          }
          //start to search for another
          pos_found2 = args.find(cmd[0][2], pos_found2+1);
        }
        for (size_t j = 2; j < cmd[0].size(); j++) {
          cmd[0].erase(cmd[0].begin()+j);
        }
        cmd[0][2] = temp_substr;
      }
    }

    if (cmd[0].size() >= 2) {
      for (size_t i = 0; i < cmd[0][1].size(); ++i) {
        //variable name only contains numbers, letters and underscore
        if ((cmd[0][1][i]>47 && cmd[0][1][i]<58) || (cmd[0][1][i]>64 && cmd[0][1][i]<91) || (cmd[0][1][i]>96 && cmd[0][1][i]<123) || (cmd[0][1][i]==95)) {
          var_name.push_back(cmd[0][1][i]);
        }
        else {
          //break if invalid character encountered
          std::cerr << "invalid variable name\n";
          return -1;
        }
      }
      //add to map of variable names and values
      //this is a pair to check if certain variable exist in a map
      std::pair<std::map<std::string, std::string>::iterator, bool> var_existence;
      //if "set var" without a value, then var_value is an empty string
      if (cmd[0][2].empty() || cmd[0].size() < 3) {
        var_existence = var_map.insert(std::pair <std::string, std::string> (var_name, ""));
      }
      else {
        var_existence = var_map.insert(std::pair <std::string, std::string> (var_name, cmd[0][2]));
      }
      //if variable is already in the map, update variable value
      if (var_existence.second == false) {
        var_map.erase(var_name);
        if (cmd[0][2].empty() || cmd[0].size() < 3) {
          var_existence = var_map.insert(std::pair <std::string, std::string> (var_name, ""));
        }
        else {
          var_existence = var_map.insert(std::pair <std::string, std::string> (var_name, cmd[0][2]));
        }
      }
    }
    //invalid set command with missing operands
    else {
      std::cerr << "invalid set operation with missing operands\n";
      return -1;
    }
    return 1;
  }

  //export variable set by user
  else if (cmd[0][0] == "export") {
    int set_status = -1;

    //check if the number of arguments is larger than 2
    if (cmd[0].size() > 2) {
      std::cerr << "too many arguments\n";
      return -1;
    }

    if (cmd[0].size() > 1) {
      std::string var_name;
      for (size_t i = 0; i < cmd[0][1].size(); ++i) {
        //variable name should only contains numbers, letters or underscores
        if ((cmd[0][1][i]>47 && cmd[0][1][i]<58) || (cmd[0][1][i]>64 && cmd[0][1][i]<91) || (cmd[0][1][i]>96 && cmd[0][1][i]<123) || (cmd[0][1][i]==95)) {
          var_name.push_back(cmd[0][1][i]);
        }
        else {
          std::cerr << "invalid variable name\n";
          return -1;
        }
      }
      //do nothing if the variable exported is unknown or in the environ
      if (var_map.size() > 0) {
        std::map<std::string, std::string>::iterator it = var_map.find(var_name);
        if (it != var_map.end()) {
          std::string var_value = var_map.find(var_name)->second;
          if (var_value.size() > 0 || var_value == "") {
            //make change to environ
            set_status = setenv(var_name.c_str(), var_value.c_str(), 1);
          }
        }
        //no variable name found in var_map, unable to export, do nothing
        else {
          return -1;
        }
      }
    }
    //if the command is just "export"
    else {
      std::cerr << "invalid variable name\n";
      return -1;
    }
    if (set_status != 0) {
      //perror("set");
      return -1;
    }
    return 1;
  }
  return 0;
}



/* search specific command program in directories stored in PATH environment
   variable if the command entered does not contain a '/', if no command is
   found, return false */
bool MyShell::search_cmd (std::string &path, std::string &cmd) {
  struct dirent *file = NULL;
  DIR *dir = NULL;
  char *dir_name = (char*)path.c_str();
  bool cmd_found = false;

  if ((dir = opendir(dir_name)) == NULL) {
    return false;
  }
  while ((file = readdir(dir)) != NULL) {
    //check if this path contains the program (indicated by cmd)
    std::string file_name(file->d_name);
    if (file_name == cmd) {
      cmd_found = true;
    }
  }
  closedir(dir);
  return cmd_found;
}



/* execute a single command using execve() system call */
void MyShell::execute_cmd (std::vector <std::string> &arglist) {
  int error;
  size_t i = 0;
  char **args = new char*[arglist.size()+1];
  char *paths;
  bool cmd_found = false;
  //external declaration of variable environ which is used in execve()
  extern char **environ;

  if (arglist.size() == 0) {
    exit(EXIT_SUCCESS);
  }
  else if (arglist[0].size() == 0) {
    exit(EXIT_SUCCESS);
  }

  //if command with no "/", search for paths from PATH
  else if(arglist[0].find("/") == std::string::npos) {
    paths = getenv("PATH");
    std::string paths_s(paths);
    std::string one_path;

    //split paths with reference to ':'
    for (i = 0; i < paths_s.size(); ++i) {
      if (cmd_found == 1) {
        break;
      }
      if (paths_s[i] == ':') {
        //search if command is within the path
        cmd_found = search_cmd(one_path, arglist[0]);
        if (cmd_found == true) { //command found
          break;
        }
        else {
          one_path.clear();
        }
      } 
      else {
        one_path.push_back(paths_s[i]);
      }
    }
    if (cmd_found == false) {
      //remember to search the last path with no ':' at the end
      cmd_found = search_cmd(one_path, arglist[0]);
    
      if (cmd_found == false) {
        std::cerr << "Command " << arglist[0] << " not found\n";
        exit(EXIT_FAILURE);
      }
      else { //command found
        //replace program with its absolute path
        one_path.push_back('/');
        arglist[0] = one_path.append(arglist[0]);
      }
    }
    else { //command found
      //replace program with its absolute path
      one_path.push_back('/');
      arglist[0] = one_path.append(arglist[0]);
    }
  }

  //assign commands and parameters to char **args
  for (i = 0; i < arglist.size(); ++i) {
    args[i] = new char[arglist[i].size()+1];
    strcpy(args[i], arglist[i].c_str());
    args[i][arglist[i].size()] = '\0';
  }
  args[arglist.size()] = (char* const)NULL;

  //execute command
  error = execve(args[0], (char* const*)args, environ);
  if (error == -1) {
    perror("execve");
    exit(EXIT_FAILURE);
  }
  //free memory or char **args
  for (i = 0; i < arglist.size(); ++i) {
    delete[] args[i];
  }
  delete args;

  exit(EXIT_SUCCESS);
}



/* redirect stdin, stdout and stderr to files with "<", ">" and "2>" respectively */
void MyShell::redirect (std::vector <std::string> &arglist) {
  int fd;

  //redirect stdin
  for (size_t i = 0; i < arglist.size(); ++i) {
    if (arglist[i] == "<") {
      if (arglist[i+1].size() > 0) {
        //open file to read
        fd = open(arglist[i+1].c_str(), O_RDONLY);
        if (fd == -1) {
          perror("-bash");
          exit(EXIT_FAILURE);
        }
        //redirect stdin to file opened
        dup2(fd, 0);
        //close old fd
        close(fd);
   
        //remove "<" and filename from arglist
        arglist.erase(arglist.begin()+i);
        arglist.erase(arglist.begin()+i);
        i = i - 1;
      }
      else {
        std::cerr << "invalid file name\n";
        exit(EXIT_FAILURE);
      }
    }
    //redirect stdout
    else if (arglist[i] == ">") {
      if (arglist[i+1].size() > 0) {
        //open file to write, if not exist, create one
        fd = open(arglist[i+1].c_str(), O_CREAT|O_RDWR, 0666);
        if (fd == -1) {
          perror("-bash");
          exit(EXIT_FAILURE);
        }
        //redirect stdout to file opened and close old fd
        dup2(fd, 1);
        close(fd);

        //remove ">" and filename from arglist
        arglist.erase(arglist.begin()+i);
        arglist.erase(arglist.begin()+i);
        i = i - 1;
      }
      else {
        std::cerr << "invalid file name\n";
        exit(EXIT_FAILURE);
      }
    }
    //redirect stderr
    else if (arglist[i] == "2>") {
      if (arglist[i+1].size() > 0) {
        //open file to write, if not exist, create one
        fd = open(arglist[i+1].c_str(), O_CREAT|O_RDWR, 0666);
        if (fd == -1) {
          perror("-bash");
          exit(EXIT_FAILURE);
        }
        //redirect stderr to file opened and close old fd
        dup2(fd, 2);
        close(fd);
  
        //remove "2>" and filename from arglist
        arglist.erase(arglist.begin()+i);
        arglist.erase(arglist.begin()+i);
        i = i - 1;
      }
      else {
        std::cerr << "invalid file name\n";
        exit(EXIT_FAILURE);
      }
    }
  }
}



/* execute multiple commands with pipes, if a single command is entered, directly
   execute it without creating pipes */
void MyShell::pipe_cmd (size_t pipe_index) {
  pid_t pid;
  //pid_t w;
  //int status;
  int pipe_fd[2];
  int error;
 
  //last command, no need to create child process
  if (pipe_index + 1 == cmd.size()) {
    redirect(cmd[pipe_index]);
    execute_cmd(cmd[pipe_index]);
  }

  //create pipe, 0 for read and 1 for write
  error = pipe(pipe_fd);
  if (error == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  //parent process
  else if (pid > 0) {
    //redirect parent's stdin to pipe_fd[0]
    dup2(pipe_fd[0], 0);
    //make sure that old read and write fds are closed
    close(pipe_fd[1]);
    close(pipe_fd[0]);
    //repeatedly execute for number of '|' plus one times
    pipe_cmd(pipe_index+1);
  }
 
  //child process
  else if (pid == 0) {
    //redirect child's stdout to pipe_fd[1]
    dup2(pipe_fd[1], 1);
    //make sure that old read and write fds are closed
    close(pipe_fd[1]);
    close(pipe_fd[0]);
    //execute command in child process
    redirect(cmd[pipe_index]);
    execute_cmd(cmd[pipe_index]);
  }
}



/* main function for executing myShell */
int main (int argc, char **argv) {
  while(1) {
    pid_t pid;
    pid_t w;
    int status;
    
    //create new MyShell entity
    MyShell myShell;
    
    //get command line, ignore empty command
    if (myShell.get_cmd() != 0) {
      continue;
    }
    
    //parse command line
    myShell.parse_cmd();
    
    //execute command line
    if (myShell.cmd_size() > 0) {
      //first, check whether the command is built-in
      if (myShell.execute_builtin(variable_map) == 0) {
        pid = fork();
        //execute commands in child process
        if (pid == 0) {
          //if not built-in, execute program with pipe
          myShell.pipe_cmd(0);
        }
        else if (pid == -1) {
          perror("fork");
          exit(EXIT_FAILURE);
        }
        //parent process (this code is from man waitpid (2))
        else {
          usleep(50000);
          //print out execution status of the process
          do {
            //wait for child process to finish
            w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            if (w == -1) {
              perror("waitpid");
              exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status)) {
              //when program exits
              std::cout << "Program exited with status " << WEXITSTATUS(status) << "\n";
            }
            else if (WIFSIGNALED(status)) {
              //when prorgam is killed by a signal
              std::cout << "Program was killed by signal " << WTERMSIG(status) << "\n";
            }
            else if (WIFSTOPPED(status)) {
              std::cout << "stopped by signal " << WSTOPSIG(status) << "\n";
            }
            else if (WIFCONTINUED(status)) {
              std::cout << "continued\n";
            }
          } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
      }
    }
  }//end while(1)
  
  return EXIT_SUCCESS;
}


