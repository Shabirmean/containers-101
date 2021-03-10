#define _GNU_SOURCE

#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024) // Stack size for the cloned child

int clearenv(void);
int child_function(void *arg);
void setup_environment(char *hostname);
void setup_fsroot(const char *folder);
void get_input_args(char **args, char *pathArgs);
struct child_config {
  int argc;
  char **argv;
  char *hostname;
};

int main(int argc, char **argv) {
  printf("Welcome to containers 101!\n");
  printf("--------------------------\n");
  if (!argc) {
    fprintf(stderr, "Required flags <h, c>. 'c' should be used at the end\n");
    return EXIT_FAILURE;
  }

  /**
   * Initialize the child configuration (hostname and the first process to run)
   * based on the arguments provided via the command line
   */
  int option = 0;
  int last_optind = 0;
  bool found_cflag = false;
  struct child_config config = {0};
  while ((option = getopt(argc, argv, "c:h:"))) {
    int idx = 0;
    if (found_cflag)
      break;

    switch (option) {
    case 'c':
      config.argc = argc - last_optind - 1;
      config.argv = &argv[argc - config.argc];
      found_cflag = true;
      break;
    case 'h':
      config.hostname = optarg;
      break;
    default:
      fprintf(stderr, "Invalid flag. Supported flags are <h, c>\n");
      return EXIT_FAILURE;
    }
    last_optind = optind;
  }

  // allocate enough memory for the new process's stack
  char *stack = 0;
  if (!(stack = malloc(STACK_SIZE))) {
    fprintf(stderr, "=> malloc failed, out of memory?\n");
    return EXIT_FAILURE;
  }

  /**
   * ------------------------------------------------------------------------
   * All the different linux namespace specific flags to be used with clone()
   */
  int FLAGS = CLONE_NEWPID |   // new process tree (id) namespace
              CLONE_NEWUTS |   // new UTS namespace (e.g: hostname)
              CLONE_NEWNS |    // new mount namespace
              CLONE_NEWIPC |   // new isolated IPC resource namespace
              CLONE_NEWNET |   // new network namespace
              CLONE_NEWCGROUP; // new control group namespace

  pid_t child_pid =
      clone(child_function, stack + STACK_SIZE, FLAGS | SIGCHLD, &config);
  /**
   * ------------------------------------------------------------------------
   */
  printf("PARENT >> PID is [%d]\n", getpid());
  printf("PARENT >> PID of my child is [%d]\n", child_pid);
  if (child_pid == -1) {
    fprintf(stderr, "=> clone() failed! %m\n");
    return EXIT_FAILURE;
  }
  int child_status = 0;
  waitpid(child_pid, &child_status, 0);
  int exit_status = WEXITSTATUS(child_status);
  return exit_status;
}

/**
 * Function passed into the clone() system call which will be the entrypoint to
 * the child process. After setting the hostname of the child process, this
 * function execve()'s to load the program passed in as the command line
 * argument to replace itself
 */
int child_function(void *arg) {
  struct child_config *config = arg;
  printf("CHILD  >> PID is [%d]\n", getpid());
  setup_environment(config->hostname);
  setup_fsroot("./rootfs");
  mount("proc", "/proc", "proc", 0, 0);

  char *args[2];
  get_input_args(args, config->argv[0]);
  printf("CHILD  >> execve(\"%s\", \"%s\", NULL)\n", args[0], args[1]);
  printf("--------------------------\n");
  if (execve(args[0], args, NULL)) {
    fprintf(stderr, "invocation to execve() failed! %m.\n");
    return -1;
  }
  return 0;
}

/**
 * Utility method used by the child process to jail itself to specific part of
 * the filesystem (in this case to the localtion of the container-image) before
 * replacing itself with the program to be run inside the child
 */
void setup_fsroot(const char *folder) {
  chroot(folder);
  chdir("/");
}

/**
 * Utility function that sets up the environment variables inside child process
 * namespace
 */
void setup_environment(char *hostname) {
  sethostname(hostname, strlen(hostname));
  clearenv();
  setenv("TERM", "xterm-256color", 0);
  setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin", 0);
}

/**
 * Simple utility method to parse the command line args into a format usable
 * with execve() system call
 */
void get_input_args(char **args, char *pathArgs) {
  int pathLen = 0;
  char *tempPtr = pathArgs;
  while (*tempPtr != '\0' && *tempPtr != ' ') {
    tempPtr++;
    pathLen++;
  }
  if (*tempPtr == ' ') {
    int argsLen = strlen(tempPtr + 1);
    args[0] = calloc(pathLen, sizeof(char));
    args[1] = calloc(argsLen, sizeof(char));
    strncpy(args[1], tempPtr + 1, argsLen);
    strncpy(args[0], pathArgs, pathLen);
  } else {
    args[0] = pathArgs;
  }
}
