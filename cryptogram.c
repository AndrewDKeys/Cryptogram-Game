#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <netdb.h>
#include <semaphore.h>
#include <pthread.h>

#define ALPHABET_LENGTH 26 //Used for the lenght of encryption key, player key, and associated loops
#define BUFFER_SIZE 1024 //sufficient size for HTTP requests and responses
#define PORT "8000"
#define THREAD_COUNT 2

char *answer; //Answer to the cryptogram
char *decryptedAnswer;
char encryptionKey[ALPHABET_LENGTH];
char playerKey[ALPHABET_LENGTH]; //Correct guesses that the player has made
int listLength; //the length of the quotes
struct quote *root; //root of linked list

//structure for threading
struct request {
    pthread_t thread;
    int client; //client's fd
    char *path; //path to the html file
    char *html;
};

void *handleRequest(void *request) {
    struct request *clientRequest = (struct request *)request;
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    int bytes = recv(clientRequest->client, buffer, BUFFER_SIZE, 0); //will put the http request into buffer
    if(bytes == -1) {
        close(clientRequest->client);
        perror("request error");
        pthread_exit(NULL);
    }


    //find the path being requested
    char *saveptr;
    char *token = strtok_r(buffer, " ", &saveptr);
    for(int i = 0; token != NULL && i < 1; i++) {
        token = strtok_r(NULL, " ", &saveptr);
    }
    token = token + 1; //this gets rid of the first '/' so we can cat it later

    //build the absolute path
    char *savedirptr;
    char *directoryWalk = strtok_r(token, "/", &savedirptr);
    while(directoryWalk != NULL) {
        printf("here\n");
        if(strstr(directoryWalk, ".html") == NULL) { //seeing if this is not the html file
            strcat(clientRequest->path, directoryWalk);
            strcat(clientRequest->path, "/");
            directoryWalk = strtok_r(NULL, "/", &savedirptr);
        } else {
            clientRequest->html = directoryWalk;
            break; //we found the html file, we don't want that in our path
        }
    }

    DIR *htmlDirectory = opendir(clientRequest->path);
    if(htmlDirectory == NULL) {
        free(clientRequest->path);
        send(clientRequest->client, "HTTP/1.1 404 Not Found\r\nContent-Length: 33\r\n\r\n<h1>404 Directory Not Found</h1>", 200, 0);
        close(clientRequest->client);
        pthread_exit(NULL);
    }

    //begin to look through the open directory to find the html file
    printf("Looking for html file\n");
    char *htmlFile;
    int fileSize;
    struct dirent *entry;
    while((entry = readdir(htmlDirectory)) != NULL) {
        if(strstr(entry->d_name, clientRequest->html) != NULL) {
            int file = open(entry->d_name, O_RDONLY);
            if(file == -1) {
                free(clientRequest->path);
                send(clientRequest->client, "HTTP/1.1 404 Not Found\r\nContent-Length: 33\r\n\r\n<h1>404 File Not Found</h1>", 200, 0);
                close(clientRequest->client);
                pthread_exit(NULL);
            }

            struct stat statbuf;
            fstat(file, &statbuf);
            fileSize = statbuf.st_size;

            htmlFile = (char *)malloc(fileSize + 1);
            htmlFile[0] = ' ';
            read(file, htmlFile, fileSize + 1);

            close(file);
            break; //we can stop looking now
        }
    }
    if(htmlFile == NULL || fileSize <= 0) {  
        send(clientRequest->client, "HTTP/1.1 404 Not Found\r\nContent-Length: 33\r\n\r\n<h1>404 File Not Found</h1>", 200, 0);
        free(clientRequest->path);
        close(clientRequest->client);    
        closedir(htmlDirectory);
        pthread_exit(NULL);
    }

    char response[50 + fileSize + 1]; //initializing enough room for the response
    response[0] = ' ';
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s", fileSize, htmlFile);
    //strcat(response, htmlFile);

    send(clientRequest->client, response, sizeof(response), 0);

    free(htmlFile);
    closedir(htmlDirectory);
    free(clientRequest->path);
    close(clientRequest->client);
    pthread_exit(NULL);
}

void requestListen(int sockfd, const char *path) {
    //set up any semaphores here
    int client;
    struct sockaddr_storage clientAddress;
    socklen_t sockSize = sizeof(clientAddress);
    while(1) {
        client = accept(sockfd, (struct sockaddr *)&clientAddress, &sockSize);
        if(client == -1) {
            perror("accept request failure");
            close(client);
            close(sockfd);
            exit(1);
        }
        
        struct request newRequest;
        newRequest.client = client;
        newRequest.path = (char *)malloc(1024*sizeof(char)); //enough for 1k bytes
        strcpy(newRequest.path, path);
        pthread_create(&newRequest.thread, NULL, handleRequest, (void *)&newRequest);
    }
}

//this will return a socketfd on succes or a -1 on failure
int socketSetUp() {
    struct addrinfo addressInfo, *serverInfo, *current; //current will be used for looping
    memset(&addressInfo, 0, sizeof(addressInfo));
    addressInfo.ai_family = AF_UNSPEC;
    addressInfo.ai_socktype = SOCK_STREAM;
    addressInfo.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, PORT, &addressInfo, &serverInfo) == -1) {
        freeaddrinfo(serverInfo);
        freeaddrinfo(current);
        perror("server address info error\n");
        exit(1);
    }

    int sock;
    for(current = serverInfo; current != NULL; current = current->ai_next) {
        sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if(sock == -1) {
            continue; //failed, trying another
        }
        if(bind(sock, current->ai_addr, current->ai_addrlen) == -1) {
            perror("binding failed");
            close(sock);
            continue; //failed, trying another
        }
        break;
    }
    free(serverInfo);

    //we will have 25 requests for every thread we are using
    if(listen(sock, THREAD_COUNT*25) == -1) {
        close(sock);
        perror("sock listen failure");
        exit(1);
    }

    return sock;
}

//Uses the Fisher-Yates method to shuffle our encryption key
void shuffle(char encryption[ALPHABET_LENGTH]) {
  srand(time(NULL)); //Random number initialization
  for (int i = 25; i > 0; i--) {
    int j = rand() % (i);
    
    //Swapping the two indices
    char temp = encryption[i];
    encryption[i] = encryption[j];
    encryption[j] = temp;
  } 
}

//linked list of each quote from "quotes.txt"
struct quote {
  char *text;
  char *author;
  struct quote *next;
};

//Initializes an empty quote
struct quote * initQuote() {
  struct quote *newQuote = (struct quote*)malloc(sizeof(struct quote));
  newQuote->text = NULL;
  newQuote->author = NULL;
  newQuote->next = NULL;
  return newQuote;
}

//Reads each quote into memory and adds it to the linked list
void loadPuzzle() {
  FILE *fp;
  fp = fopen("quotes.txt", "rb");
  listLength = 0; //initializes listLength

  char *buf = (char *)malloc(150); //allocates more than enough memory than we need to read a line
  struct quote *newQuote = initQuote();
  while(fgets(buf, 150, fp) != NULL) {
    if(strlen(buf) < 3) { // Empty lines mean that is is the end of the quote
      if((listLength++) == 0) {
        root = newQuote;
      }
      newQuote->next = initQuote();
      newQuote = newQuote->next;
    } else if(buf[0] == '-' && buf[1] == '-') {
      newQuote->author = strdup(buf);
    } else if(newQuote->text == NULL) { //if it is the first line in the quote
      newQuote->text = strdup(buf);
    } else { //adding text onto the end of the quote
      char *temp = (char *)malloc(strlen(newQuote->text) + 1); //temp holds the old value of newQuote
      strcpy(temp, newQuote->text);
      free(newQuote->text); //getting ready for redistribution
      newQuote->text = (char *)malloc(strlen(temp) + strlen(buf) + 1); //make new space for quote
      strcpy(newQuote->text, temp); //copy temp back over
      free(temp);
      newQuote->text = strcat(newQuote->text, buf); //make the new quote
    }
  }
  newQuote = NULL; //signifies the end of the list
  fclose(fp);
  free(buf);
}


char* getPuzzle() {
  struct quote *current = root;
  srand(time(NULL));
  int loop = rand() % (listLength); //gets a random quote from the linkedlist
  for(int i = 0; i < loop; i++) {
    current = current->next;
  }
  return current->text;
}

char* acceptInput() {
  printf("Enter a letter and then a letter to replace it with or 'quit' to quit.\n");
  
  char *input = (char *)malloc(100);
  fgets(input, sizeof(input), stdin);
  
  //Finds the return carriage and new line of the string to get rid of.
  char *returnCarriage = strchr(input, '\r'); //finds first '\r'
  char *newLine = strchr(input, '\n'); //finds first '\n'
  if(returnCarriage != NULL) {
    *returnCarriage = '\0';
  }
  if(newLine != NULL) {
    *newLine = '\0';
  }

  return input;
}

//We want to return true on 'quit' or replace a letter in the answer
bool updateState(char *input) {
  if(strlen(input) == 2) { 
    playerKey[toupper(*(input)) - 'A'] = toupper(*(input + 1)); //Uses distance to replace the player's key with the respective character
    return false;
  } else if((strcmp(input, "quit") == 0) || input == NULL) {
    return true;
  } else {
    printf("\nError reading input, please try again.\n");
    return false; 
  }    
}

//returns whether or not they have won
bool displayWorld() {
  bool win = true; //stays true until an incorrect character is guessed
  printf("\nEncrypted:\n%s\n", answer);
  printf("Decrypted:\n");
  for(int i = 0; i < strlen(answer); i++) {
    if(isalpha(answer[i])) { //only if it is an alphabetic character
      if(playerKey[answer[i]-'A'] != '\0') {
        printf("%c", playerKey[answer[i] - 'A']); //player's guessed character
        if(playerKey[answer[i] - 'A'] != toupper(decryptedAnswer[i])) {
          win = false;
        }
      } else {
        printf("_"); //player hasn't guessed this one yet
        win = false;
      }
    } else {
      printf("%c", answer[i]);
    }
  }
  return win;
}

//set the value for the answer
void initialization() {
  if(root == NULL) {
    loadPuzzle();
  }
  answer = strdup(getPuzzle());
  decryptedAnswer = strdup(answer);
  
  //Filling the encryptionKey with the English alphabet
  encryptionKey[0] = 'A';
  for(int i = 1; i < ALPHABET_LENGTH; i++) {
    encryptionKey[i] = encryptionKey[i-1] + 1; 
  }
  shuffle(encryptionKey);
    
  //Fills playerKey with NUL terminator to prevent NULL pointing
  for(int i = 0; i < ALPHABET_LENGTH; i++) {
    playerKey[i] = '\0';
  }
  //checks to see if the character is alphabetic
  //Gets the encrypted letter, we subtract 'A' to find the letter's index
  for(int i = 0; answer[i] != '\0'; i++) {
    if(isalpha(answer[i])) {
      answer[i] = encryptionKey[toupper(answer[i]) - 'A'];
    }     
  }
}

void gameLoop() {
  char *str = (char *)malloc(100);
  bool win;
  do {
    win = displayWorld(); //Prints string to terminal
    if(win) {
      break;
    }
    char *input = acceptInput(); //Accepts user's guess
    strcpy(str, input);
    free(input);
  } while(!updateState(str)); //Loops until 'quit' or win
  if(win) {
    printf("Good job! You succesfully decrypted the quote!\n");
  }
  free(str);
}

bool playAgain() {
  char *input = (char *)malloc(10);
  fgets(input, sizeof(input), stdin);
  //Finds the return carriage and new line of the string to get rid of.
  char *returnCarriage = strchr(input, '\r'); //finds first '\r'
  char *newLine = strchr(input, '\n'); //finds first '\n'
  if(returnCarriage != NULL) {
    *returnCarriage = '\0';
  }
  if(newLine != NULL) {
    *newLine = '\0';
  }

  if(*input == 'y' || *input == 'Y') {
    free(input);
    return true;
  } else {
    free(input);
    return false;
  }
}

void freeList() {
  struct quote *current = root;
  while(current != NULL) {
    struct quote *next = current->next;
    if(current->text != NULL) {free(current->text);}
    if(current->author != NULL) {free(current->author);}
    free(current);
    current = next;
  }
  root = NULL;
}

void teardown() {
  free(answer);
  free(decryptedAnswer);
  printf("All Done\n");
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("failure: no path provided\n");
        return EXIT_FAILURE;
    }

    int sockfd = socketSetUp();
    if(sockfd == -1) {
        close(sockfd);
        perror("network setup error\n");
        return EXIT_FAILURE;
    }

    requestListen(sockfd, argv[1]);

  // do {
  //   initialization();
  //   gameLoop();
  //   teardown();
  //   printf("Would you like to play again? [y/n]\n");
  // } while(playAgain());
  // freeList();

  return 0;
}
