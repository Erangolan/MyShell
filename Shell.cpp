#include <iostream>
#include <string>
#include <list>
#include <iterator>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <iterator>
#include <cstddef>
#include <cstring>
using namespace std;

static int fd;
static fpos_t pos;

void switchStdout(const char *newStream);
void revertStdout();
void switchStdin(const char *newStream);
void revertStdin();
void env(char** args, list<string>& en);
void parse(char *line, char **args);
void execute(char **args, list <pid_t>& pidList, list <string>& path, list <string>& history);
void task (list<pid_t> &pidList, list<string>& path);
void print_env (list<string>& en);
void printHistory (list<string>& history);


int main(int argc, char *argv[]) {
  char line[1024];
  list <pid_t> pidList;
  list <string> path;
  list<string> en;
  list<string> history;
  cout << '>' << ' ';
  while (fgets(line, 1024, stdin) != 0) {
      char* args[1024] = {0};
      line[strlen(line) - 1] = 0;
      cout << '>' << ' ';
      if (line[0] != '/')
      {
        if (strcmp(line, "print_history") == 0){
            printHistory(history);
            return 0;
        }
        if (strcmp(line, "print_env") == 0)
          print_env(en);
        else
        {
          if (strcmp(line, "tasks") == 0)
              task(pidList, path);
          else
          {
            parse(line, args);
            env(args, en);
          }
        }
      }
      else
      {
        parse(line, args);
        execute(args, pidList, path, history);
      }
    }
  return 0;
}

void parse(char *line, char **args)
{
    while (*line != '\0') {
        while (*line == ' ' || *line == '\t' || *line == '\n')
              *line++ = '\0';
          *args++ = line;
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
              line++;
     }
}

void execute(char **args, list <pid_t>& pidList, list <string>& path, list <string>& history)
{
    pid_t  pid;
    int    status, fd;
    fpos_t pos;

    if ((pid = fork()) < 0){
      perror("fork");
      return;
    }

    pidList.push_back(pid);
    path.push_back(args[0]);
    int i = 1;
    while(args[i] != 0)
        path.back() += args[i++];

    history.push_back(path.back());

    if (i > 1 && strcmp(args[i-2], "<") == 0){
      if(!pid){
        switchStdin(args[i-1]);
        args[i-1] = args[i-2] = NULL;
        if(execvp(args[0], &args[0]) < 0){
          perror("execvp");
          return;
        }
      }
      if (pid){
        if((pid == waitpid(pid, &status, 0) < 0)){
          perror("waitpid");
          return;
        }
        revertStdin();
      }
    }

    if (i > 1 && strcmp(args[i-2], ">") == 0){
      if(!pid){
        switchStdout(args[i-1]);
        args[i-1] = args[i-2] = NULL;
        if(execvp(args[0], &args[0]) < 0){
          perror("execvp");
          return;
        }
      }
      if (pid){
        if((pid == waitpid(pid, &status, 0) < 0)){
          perror("waitpid");
          return;
        }
        revertStdout();
      }
    }

    if (strcmp(args[i-1], "&") == 0){
      if(!pid){
        args[i-1] = NULL;
        if(execvp(args[0], &args[0]) < 0){
          perror("execvp");
          return;
        }
      }
    }

    if (strcmp(args[i-1], "&") != 0){
      if (!pid){
        if(execvp(args[0], &args[0]) < 0){
          perror("execvp");
          return;
        }
      }
      if (pid){
        pidList.pop_back();
        path.pop_back();
        if((pid == waitpid(pid, &status, 0) < 0)){
          perror("waitpid");
          return;
        }
      }
    }
}


void printHistory (list<string>& history)
{
  int count = 1;
  list<string>::const_iterator it = history.cbegin();
  while (it != history.cend())
    cout << count++ << ") " << *it++ << endl;
}

void switchStdin(const char *newStream)
{
  fflush(stdin);
  fgetpos(stdin, &pos);
  fd = dup(fileno(stdin));
  freopen(newStream, "r", stdin);
}

void revertStdin()
{
  fflush(stdin);
  dup2(fd, fileno(stdin));
  clearerr(stdin);
  fsetpos(stdin, &pos);
}

void switchStdout(const char *newStream)
{
  fflush(stdout);
  fgetpos(stdout, &pos);
  fd = dup(fileno(stdout));
  freopen(newStream, "w", stdout);
}

void revertStdout()
{
  fflush(stdout);
  dup2(fd, fileno(stdout));
  clearerr(stdout);
  fsetpos(stdout, &pos);
}

void env(char** args, list<string>& en)
{
  const char s[2] = "=";
  char *token1, *token2;
  token1 = strtok(*args, s);
  token2 = strtok(NULL, s);
  setenv(token1, token2, 0);
  en.push_back(token1);
}

void task (list<pid_t> &pidList, list<string>& path)
{
  list<pid_t>::const_iterator it1 = pidList.cbegin();
  list<string>::const_iterator it2 = path.cbegin();
  while (it2 != path.cend())
      cout << *it2++ << ": " << *it1++ << endl;
}

void print_env (list<string>& en){
  list<string>::const_iterator it = en.cbegin();
  while (it != en.cend()){
      cout << *it << ":\t" << getenv((*it).c_str()) << endl;
      it++;
  }
}