#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_execute(char **args);
int lsh_launch(char **args);
char ***lsh_split_pipeline(char *line, int *nums_cmds);
int lsh_execute_pipline(char ***cmds, int num_cmds);

char *buildin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*buildin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_buildins(){
    return sizeof(buildin_str)/sizeof(char *);
}

int lsh_cd(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else{
        if(chdir(args[1]) != 0){
            perror("lsh");
        }
    }

    return 1;
}

int lsh_help(char **args){
    int i;
    printf("HiggsBoson's LSH\n");
    printf("Type program names and arguments and hit error\n");
    printf("The following are build in: \n");

    for(i = 0; i < lsh_num_buildins(); i++){
        printf("%s\n", buildin_str[i]);
    }

    printf("use the man command for information on other programs as well\n");
    return 1;
}

int lsh_exit(char **args){
    return 0;
}

char *lsh_read_line(void){
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if(!buffer){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        //read a character one at a time;
        c = getchar();

        //if we hit EOF, replace it with a null character and return;
        if(c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        }
        else{
            buffer[position] = c;
        }
        position++;

        //if we hvae exceeded the buffer, reallocate
        if(position >= bufsize){
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, sizeof(char) * bufsize);
            if(!buffer){
                fprintf(stderr, "lsh: allocation  error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **lsh_split_line(char* line){
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if(!tokens){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while(token != NULL){
        tokens[position] = token;
        position++;

        if(position >= bufsize){
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens){
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int lsh_execute(char **args){
    int i;

    if(args[0] == NULL){
        return 1;
    }

    for(i = 0; i < lsh_num_buildins(); i++){
        if(strcmp(args[0], buildin_str[i]) == 0){
            return (*buildin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

int lsh_launch(char **args){
    pid_t pid, wpid;
    int status;

    //working on parse redirection operators
    int i = 0;
    char *output_file = NULL;
    char *input_file  = NULL;
    int  append_mode  = 0;

    while( args[i] != NULL){
        if(strcmp(args[i], ">") == 0){
            output_file = args[i + 1];
            append_mode = 0;
            args[i] = NULL;
            break;
        }
        else if(strcmp(args[i], ">>") == 0){
            output_file = args[i + 1];
            append_mode = 1;
            args[i] = NULL;
            break;
        } 
        else if(strcmp(args[i], "<") == 0){
            input_file = args[i + 1];
            args[i] = NULL;
            break;
        }
        i++;
    }
    //start a childprocess
    pid = fork();
    if(pid == 0){
        //a reminder to wire up the file descriptors before calling the exec 

        if(input_file != NULL){
            int fd_in = open(input_file, O_RDONLY);
            if(fd_in < 0) {
                perror("lsh: open input");
                exit(EXIT_FAILURE);
            }
            //replacing the stdin with file
            //dup2 makes the child process's file descriptor 1 point to your file
            //this means that when execvp runs the new program, it inherits those fds, so all its printf/wirte calls go to your file
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if(output_file != NULL){
            int flags = O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC);
            //need to understand why are we using this 0644 
            int fd_out = open(output_file, flags, 0644);
            if(fd_out < 0) {
                perror("lsh: open output");
                exit(EXIT_FAILURE);
            }
            //REPLACING THE STDOUT WITH THE FILE
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        if(execvp(args[0], args) == -1) { perror("lsh"); }
        exit(EXIT_FAILURE);
    }
    else if(pid < 0){
        perror("lsh");
    }
    else{
        do{
            wpid = waitpid(pid, &status, WUNTRACED);
        }while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

//basically splitting a command line into an array of arguments arrays, split on "|"
//e.g. "ls -l | grep foo | wc -l" → [ ["ls","-l",NULL], ["grep","foo",NULL], ["wc","-l",NULL] ]
//the above is the best exmple to do this 
char ***lsh_split_pipeline(char *line, int *nums_cmds){
    //frist split on "|" to get command strings
    char **pipe_parts = malloc(LSH_TOK_BUFSIZE * sizeof(char *));
    int count = 0;

    char *segments = strtok(line, "|");
    while(segments != NULL){
        pipe_parts[count++] = segments;
        segments = strtok(NULL, "|");
    }

    //in the segments are split into arguments
    char ***cmds = malloc(count * sizeof(char **));
    for(int i = 0; i < count; i++){
        cmds[i] = lsh_split_line(pipe_parts[i]);
    }

    free(pipe_parts);
    *nums_cmds = count;
    return cmds;
}

int lsh_execute_pipline(char ***cmds, int num_cmds){
    if(num_cmds == 1){
        //no pipe_fall thorugh to normal execute
        return lsh_execute(cmds[0]);
    }

    int pipes[num_cmds - 1][2];

    for(int i = 0; i < num_cmds - 1; i++){
        if(pipe(pipes[i]) < 0){
            perror("lsh: pipe");
            return 1;
        }
    }

    pid_t pids[num_cmds];

    for(int i = 0; i < num_cmds; i++){
        pids[i] = fork();

        if(pids[i] == 0){
            //child i

            if(i > 0){
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            if(i < num_cmds - 1){
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for(int j = 0; j < num_cmds - 1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if(execvp(cmds[i][0], cmds[i]) == -1){
                perror("lsh");
            }
            exit(EXIT_FAILURE);
        } else if (pids[i] < 0){
            perror("lsh: fork");
        }
    }

    for(int i = 0; i < num_cmds - 1; i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for(int i = 0; i < num_cmds; i++){
        int status;
        waitpid(pids[i], &status, 0);
    }
    return 1;
}

void lsh_loop(void){
    char *line;
    int num_cmds;
    int status;

    do{
        printf("> ");
        line = lsh_read_line();

        char ***cmds = lsh_split_pipeline(line, &num_cmds);
        status = lsh_execute_pipline(cmds, num_cmds);

        for(int i = 0; i < num_cmds; i++) free(cmds[i]);
        free(cmds);
        free(line);
    } while(status);
}

int main(){
    lsh_loop();

    return EXIT_SUCCESS;
}

