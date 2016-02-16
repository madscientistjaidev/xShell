#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

//Environment variables
#define lineSize 80
#define bufferMax 1024
#define tokenSize 64
#define delimiters " \t\r\n\a\""
#define nativeCount 6
#define true 1
#define false 0
#define	failure 1
#define	success 0

//Prototypes for native commands.
int changeDir(char **args);
int listNative(char **args);
int doExit(char **args);
int printHistory();
int doLast();
int createAlias();

//For reading current path.
char *getcwd(char *buf, size_t size);

//Structures for holding history and alias information.
char *history[lineSize];
int historySize=0;
char *aliasList[lineSize];
int aliasCount=0;

//Native commands.
char *nativeStrings[] = {"cd", "help", "exit", "history", "!!", "alias"};
int (*nativeCommands[]) (char **) = {&changeDir, &listNative, &doExit, &printHistory, &doLast, &createAlias};

//Changes directory.
int changeDir(char **args)
{
    //Empty path.
    if (args[1] == NULL)
    {
        printf("Path expected\n");
        return 1;
    }
    
    if (chdir(args[1]) != 0) printf("Path not found\n");
    
    return 1;
}

//Lists native commands.
int listNative(char **args)
{
    for (int i = 0; i < nativeCount; i++) printf("%s\t", nativeStrings[i]);
    
    printf("\n");
    
    return 1;
}

//Exits shell.
int doExit(char **args) {return 0;}

//Prints history.
int printHistory(char **args)
{
    for(int i=historySize-1; i>=0; i--)
        printf("%d %s \n", i, history[i]);
    
    return 1;
}

//Runs most recent command.
int doLast(char **args)
{
    char **arg = history[historySize-1];
    
    return doCommand((arg));
    return 1;
}

//Creates command alias.
int createAlias(char **args)
{
    aliasList[aliasCount][0] = malloc(strlen(args[1]) + 1);
    strcpy(aliasList[aliasCount][0], args[1]);
    
    aliasList[aliasCount][1] = malloc(strlen(args[2]) + 1);
    strcpy(aliasList[aliasCount][1], args[2]);
        
    aliasCount++;
    
    printf("%s %s\n", args[1], args [2]);
    return 1;
}

//Executes OS commands.
int doCommand(char **args)
{   
    pid_t procID = fork();
    
    //Return to child.
    if (procID == 0)
    {
        if (execvp(args[0], args) == -1) perror("");   
        exit(failure);
    } 
    
    //Could not create child.
    else if (procID < 0) perror("");
    
    //Waiting.
    else
    {
        // Parent process
        int flag;
        
        do waitpid(procID, &flag, WUNTRACED);
        while (!WIFEXITED(flag) && !WIFSIGNALED(flag));
    }

    return 1;
}

//Checks for native command
int parseCommand(char **args)
{
    //Blank command
    if (args[0] == NULL) return 1;

    //Compares all native commands to input.
    for (int i = 0; i < nativeCount; i++)
        if (strcmp(args[0], nativeStrings[i]) == 0)
            return (*nativeCommands[i])(args);

    //Runs OS command.
    return doCommand(args);
}

//Reads input from command line.
char *getLine(void)
{
    int bufferSize = bufferMax;
    int bufferPointer = 0;
    char *buffer = malloc(sizeof(char) * bufferSize);
    int currentChar;

    //If could not create buffer.
    if (!buffer)
    {
        fprintf(stderr, "Buffer error.\n");
        exit(failure);
    }

    while (true)
    {
        currentChar = getchar();

        //End of line.
        if (currentChar == EOF || currentChar == '\n')
        {
            buffer[bufferPointer] = '\0';
            return buffer;
        }
        
        else buffer[bufferPointer] = currentChar;

        bufferPointer++;

        //Dealing with buffer overflow.
        if (bufferPointer >= bufferSize)
        {
            bufferSize += bufferMax;
            buffer = realloc(buffer, bufferSize);
            
            //Could not reallocate buffer.
            if (!buffer)
            {
                fprintf(stderr, "Buffer error.\n");
                exit(failure);
            }
        }
    }
}

//Tokenizes input.
char **tokenizeLine(char *line)
{
    int listSize = tokenSize;
    int listPointer = 0;
    
    char **tokenList = malloc(listSize * sizeof (char*));
    char *currentToken, **listCopy;

    //Could not create list.
    if (!tokenList)
    {
        fprintf(stderr, "Buffer error.\n");
        exit(failure);
    }
    
    //Discards delimiter characters.
    currentToken = strtok(line, delimiters);
    
    while (currentToken != NULL)
    {        
        tokenList[listPointer] = currentToken;
        listPointer++;

        //Reallocates list if overflowing.
        if (listPointer >= listSize)
        {
            listSize += tokenSize;
            listCopy = tokenList;
            tokenList = realloc(tokenList, listSize * sizeof (char*));
            
            if (!tokenList)
            {
                free(listCopy);
                fprintf(stderr, "Buffer error.\n");
                exit(failure);
            }
        }

        currentToken = strtok(NULL, delimiters);
    }
    
    tokenList[listPointer] = NULL;
    return tokenList;
}

//Main function.
int main(int argc, char **argv)
{
    char *line;
    char **args;
    char currentDirectory[bufferMax];
    
    //Run flag.
    int shouldRun=1;
    
    do
    {   
        //Gets current directory.
        getcwd(currentDirectory, sizeof(currentDirectory));
        
        printf("osh$%s> ", currentDirectory);
        line = getLine();
        args = tokenizeLine(line);
        shouldRun = parseCommand(args);
        
        //Writes commands to history.
        history[historySize] = malloc(strlen(line) + 1);
        strcpy(history[historySize], line);
        historySize++;

        //Clears input stream.
        free(line);
        free(args);
    }
    while (shouldRun);

    return 0;
}