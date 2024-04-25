#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

//Used for the lenght of encryption key, player key, and associated loops
#define ALPHABET_LENGTH 26

char *answer; //Answer to the cryptogram
char *decryptedAnswer;
char encryptionKey[ALPHABET_LENGTH];
char playerKey[ALPHABET_LENGTH]; //Correct guesses that the player has made
int listLength; //the length of the quotes
struct quote *root; //root of linked list

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
  do {
    initialization();
    gameLoop();
    teardown();
    printf("Would you like to play again? [y/n]\n");
  } while(playAgain());
  freeList();

  return 0;
}
