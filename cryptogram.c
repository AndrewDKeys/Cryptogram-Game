#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

char *answer; //Answer to the cryptogram
char encryptionKey[26];
char playerKey[26]; //Correct guesses that the player has made

//Uses the Fisher-Yates method to shuffle our encryption key
void shuffle(char encryption[26]) {
  srand(time(NULL)); //Random number initialization
  for (int i = 25; i > 0; i--) {
    int j = rand() % (i);
    
    //Swapping the two indices
    char temp = encryption[i];
    encryption[i] = encryption[j];
    encryption[j] = temp;
  } 
}

void loadPuzzle() {}

char* getPuzzle() {
  char *puzzle = (char *)malloc(strlen("The 23 quick brown fox, jumped over the lazy dog") + 1); //+1 for the nul terminator
  strcpy(puzzle,"The 23 quick brown fox, jumped over the lazy dog\0");
  return puzzle;
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
    playerKey[toupper(*input) - 'A'] = toupper(*(input + 1)); //Uses distance to replace the player's key with the respective character
    return false;
  } else if((strcmp(input, "quit") == 0) || input == NULL) {
    return true;
  } else {
    printf("Error reading input, please try again.\n");
    return false; 
  }    
}

void displayWorld() {
  printf("Encrypted: %s\n", answer);
  printf("Decrypted: ");
  for(int i = 0; i < strlen(answer); i++) {
    if(isalpha(answer[i])) { //if it not a space
      if(playerKey[answer[i]-'A'] != '\0') {
        printf("%c", playerKey[answer[i] - 'A']); //player's guessed character
      } else {
        printf("_"); //player hasn't guessed this one yet
      }
    } else {
      printf("%c", answer[i]);
    }
  }
  printf("\n");

}

//set the value for the answer
void initialization() { 
  answer = getPuzzle();
  
  //Filling the encryptionKey with the English alphabet
  encryptionKey[0] = 'A';
  for(int i = 1; i < 26; i++) {
    encryptionKey[i] = encryptionKey[i-1] + 1; 
  }
  shuffle(encryptionKey);
    
  //Fills playerKey with NUL terminator to prevent NULL pointing
  for(int i = 0; i < 26; i++) {
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

  do {
    displayWorld(); //Prints string to terminal
    char *input = acceptInput(); //Accepts user's guess
    strcpy(str, input);
    free(input);
  } while(!updateState(str)); //Loops until 'quit'
  displayWorld();

  free(str);
}

void teardown() {
  free(answer);
  printf("All Done\n");
}

int main(int argc, char *argv[]) {

  initialization();
  gameLoop();
  teardown();

  return 0;
}
