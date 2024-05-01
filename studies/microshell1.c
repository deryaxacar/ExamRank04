#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int err(char *str)
{
    while(*str)
        write(2, str++, 1);
    write(2, "\n", 1);
    return 1;
}

int exec(char **av, int i)
{
    int fd[2];
    int status;
    int has_pipe = av[i] && !strcmp(av[i], "|");

    if(!has_pipe)
        return err("error");

    if(has_pipe && pipe(fd) == -1)
        return err("error: fatal\n");
    
    int pid = fork();
    if(!pid)
    {    
        argv[i] = 0;
        if (has_pipe && (dup2(fd[1], 1) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
            return err("error: fatal\n");
        if (!strcmp(*argv, "cd"))
            return cd(argv, i);
        execve(*argv, argv, __environ);
        return err("error: cannot execute "), err(*argv), err("\n");
    }

    waitpid(pid, &status, 0);
    if (has_pipe && (dup2(fd[0], 0) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
        return err("error: fatal\n");
    return WIFEXITED(status) && WEXITSTATUS(status);
}

int main(int ac, char **av)
{
    int i = 0, status = 0;

    if(ac > 1)
    {
        while(av[i] && av[++i])
        {
            av += i;
            i = 0;
            while(av[i] && strcmp(av[i], "|") && strcmp(av[i], ";"))
                i++;
            
            if (!strcmp(av[0], "cd"))
            {
                if (i != 2)
                    err("error: cd: bad arguments", 0);
                else if (chdir(av[1]) != 0)
                    err("error: cd: cannot change directory to ", av[1]);
            }
            else if (i)
                status = exec(av, i);
        }
    }
    return status;
}
