//
// Project 1 - Tug-Of-War
//
// Code by Andrew R McHugh
// Gadgets, Carnegie Mellon Univeristy, Hudson


/*
   Game Mechanics
   --------------

   Two players mash their buttons as fast as possible
   This creates a tug-of-war with the LED "rope"
   When one player gets a point, the board flashes and points to them with an arrow
   When one player wins (first to three points), the board flashes and shows the number of the winning player (left = 1, right = 2)
   Music plays in the background, randomly generated from the E minor
   Music speed increases as either player gets closer to winning a point

*/

// Include libraries
// Letters for the displayed patterns
// Pitches for the music notes
#include "letters.h"
#include "pitches.h"

// Set global variables
// Devmode turns on Serial.print() comments
bool DEVMODE = false;
// Mute mutes the speaker
bool MUTE = false;

// STATE is a variable that controls the global state of the game
enum States {
  startup,
  play,
  playerWin,
  win,
  postGame,
  score
};
States STATE;

// Setup the pins, mostly in arrays
const int ledPinsCols[] = {11, A5, 10, A0, 3, 9, 4, 7};
const int ledPinsRows[] = {A4, A3, 1, A2, A1, 6, 5, 0};//0
const int btnPins[] = {12, 13};
const int musicPin = 8;

// Define the musical scale to randomly pull from
int music[] {
  // E Minor
  NOTE_E3, NOTE_G3, NOTE_A3, NOTE_B4, NOTE_D4, NOTE_E4, NOTE_G4, NOTE_A4
};
// Number of notes in array
const int noteCount = 8;
// Initializing the position of the current note
int notePosition = 0;

// Durration of notes before speed adjustments, 4 = a quarter note
int noteDurations[] = {
  4, 4, 4, 4, 4, 4, 4, 4
};
// Initialize other music variables
int noteDuration; //defined later in the code
int musicSpeed = 1; //1 is normal, 2 is kinda-fast, three is fast

// Defined button labels for ease of use and increased readability
const int LEFT = 0;
const int RIGHT = 1;

// Set the initial states and last states
int btnStates[] = {HIGH, HIGH}; //{left, right}
int btnStatesLast[] = {LOW, LOW};

// Set the initial player scores
int playerScore[] = {1, 1}; // round scores, to be updated per round, based on the math, they start at 1
int playerScoreInit[] = {1, 1}; // defined initial round scores
int playerScoreMaster[] = {0, 0}; // the master score that can change the overall game state

// Initialize the who won last and who the game winner is
int playerWinLast;
int playerWinner;

// Initialize the total score and win location
int totalScore;
int winLocation; // defines the location of the "rope" on the led matrix

// Initialize time variables, definitions follow in code
unsigned long timeStart;
unsigned long timeLimit;
unsigned long timeMeter;
unsigned long masterClock;
unsigned long lastNotePlayedClock;

// Initialize the playArray which gets drawn on the board
int playArray[8][8];

//
// Updates button states. Sets current-stored to
// last then reads current states
//
void updateBtnStates() {
  btnStatesLast[LEFT] = btnStates[LEFT];
  btnStatesLast[RIGHT] = btnStates[RIGHT];

  btnStates[LEFT] = digitalRead(btnPins[LEFT]);
  btnStates[RIGHT] = digitalRead(btnPins[RIGHT]);
}

// Draws a given array onto the dot matrix. If
// passed an optional variable "inverse", the
// function draws the inverse of the array.
void draw(const int dotArray[8][8], bool inverse = false) {
  
  // Initialize a this variable
  int thisLED = 0;

  // Loop that draws the given array onto the matrix
  for (int thisRow = 0; thisRow < 8; thisRow++) {
    digitalWrite(ledPinsRows[thisRow], LOW);
    for (int thisCol = 0; thisCol < 8; thisCol++) {

      thisLED = dotArray[thisRow][thisCol];

      // checks inverse variable to either make 1s or 0s the drawn dot
      if (inverse == false) {
        if (thisLED == 1) {
          digitalWrite(ledPinsCols[thisCol], HIGH);
        }
      } else if (inverse == true) {
        if (thisLED == 0) {
          digitalWrite(ledPinsCols[thisCol], HIGH);
        }
      }

      // Magic sauce
      digitalWrite(ledPinsCols[thisCol], LOW);
    }
    digitalWrite(ledPinsRows[thisRow], HIGH);
  }

}

// draw() for a specified amount of time (without interupt)
void drawFor(const int dotArray[8][8], int mil, bool inverse = false) {

  // Set time and given duration
  timeStart = millis();
  timeLimit = timeStart + mil;
  timeMeter = timeStart;

  // While the counted time is less than the desired limit, keep drawing the array
  while (timeMeter < timeLimit) {
    // Draw the array
    draw(dotArray, inverse);
    // Update the counted time
    timeMeter = millis();
  }
}

// Specific "draw" for setting the matrix on
void setMatrixOn() {
  for (int thisCol = 0; thisCol < 8; thisCol++) {
    digitalWrite(ledPinsCols[thisCol], HIGH);
  }
  for (int thisRow = 0; thisRow < 8; thisRow++) {
    digitalWrite(ledPinsRows[thisRow], LOW);
  }
}

// Specific "draw" for setting the matrix off
void setMatrixOff() {
  for (int thisCol = 0; thisCol < 8; thisCol++) {
    digitalWrite(ledPinsCols[thisCol], LOW);
  }
  for (int thisRow = 0; thisRow < 8; thisRow++) {
    digitalWrite(ledPinsRows[thisRow], HIGH);
  }
}

// A set of development tests, controlled by case switch
// Not used for production
void testMatrix(int test) {
  switch (test) {
    case 1:
      for (int thisCol = 0; thisCol < 8; thisCol++) {
        for (int thisRow = 0; thisRow < 8; thisRow++) {
          digitalWrite(ledPinsRows[thisRow], LOW);
          digitalWrite(ledPinsCols[thisCol], HIGH);
          delay(20);
          digitalWrite(ledPinsRows[thisRow], HIGH);
          digitalWrite(ledPinsCols[thisCol], LOW);
        }
      }
      break;
    case 2:
      draw(TEST);
      break;
    default:
      setMatrixOn();
      delay(100);
      setMatrixOff();
  }
}

// Turn just one LED on
void ledOn(int row, int col, bool setMatrixOffVar = false) {
  // Check to see if we should start by turning the whole matrix off
  if (setMatrixOffVar == true) {
    setMatrixOff();
  }
  digitalWrite(ledPinsCols[col], HIGH);
  digitalWrite(ledPinsRows[row], LOW);
}

// Turn just one LED off
void ledOff(int row, int col, bool setMatrixOnVar = false) {
  // Check to see if we should start by turning the whole matrix on
  if (setMatrixOnVar == true) {
    setMatrixOn();
  }
  digitalWrite(ledPinsCols[col], LOW);
  digitalWrite(ledPinsRows[row], HIGH);
}
// Update the scores and redraw matrix accordingly
void updateScores() {
  // See who is pressing their button
  // Only adds one point per press, holding doesn't work here
  for (int side = 0; side < 2; side++) {
    if ((btnStates[side] != btnStatesLast[side]) && (btnStates[side] == HIGH)) {
      playerScore[side]++;
      if (DEVMODE == true) {
        Serial.print("Player Score ");
        Serial.print(side);
        Serial.print(playerScore[side]);
        Serial.println();
        delay(10); //edit
      }
    }
  }
  // Can update the buttons states now
  updateBtnStates();

  // Read scores
  // Draw scores based on "winningness"
  // If score is high enough, change state to win

  // The numbers in this if statement are "magic numbers" I played with them and they
  // seem to give an enjoyable experience with variability.
  if ((playerScore[LEFT] > 15) || (playerScore[RIGHT] > 15)) {
    // Because their score gets mapped to a location on the display, winning requires
    // a higher percentage, not pure number. I thus decrement the score (which changes
    // the mapping behavior and constrain this adjustment to keep some stability.
    playerScore[LEFT]  = constrain(playerScore[LEFT]  - 4, 0, playerScore[LEFT] - 4);
    playerScore[RIGHT] = constrain(playerScore[RIGHT] - 4, 0, playerScore[RIGHT] - 4);
  }
  totalScore = playerScore[LEFT] + playerScore[RIGHT];
  // Maps the scores to a usable location. -1 and 9 are the win states
  winLocation = map(playerScore[RIGHT], 0, totalScore, -1, 9);
  
  // Update the music speed based on their "winningness" location on the matrix
  updateMusicSpeed(winLocation);

  // Dev function
  if (DEVMODE == true) {
    Serial.println(winLocation);
  }
  // Win state
  if (winLocation == -1) {
    playerScoreMaster[LEFT] += 1;
    playerWinLast = LEFT;
    STATE = playerWin;
  }
  // Win state
  else if (winLocation == 9) {
    playerScoreMaster[RIGHT] += 1;
    playerWinLast = RIGHT;
    STATE = playerWin;
  }
  // If you don't win, draw the matrix again
  else {
    for (int j = 0; j < 8; j++) {
      for (int i = 0; i < 8; i++) {
        // Selectively draw two lines for the pure center of the matrix
        if ((winLocation == 4) && (j == winLocation)) {
          playArray[i][j] = 1;
          playArray[i][j - 1] = 1;
        }
        // Otherwise draw a single line on either side of center
        else if ((winLocation < 4) && (j == winLocation)) {
          playArray[i][j] = 1;
        }
        else if ((winLocation > 4) && (j == winLocation - 1)) {
          playArray[i][j] = 1;
        }
        else {
          playArray[i][j] = 0;
        }
      }
    }
    // Now that you've prepared the array, draw it
    draw(playArray);
  }
}

// When a player wins a round...
void playerWinSingle() {
  // Flashes the board and points to who won the round
  drawFor(OFF, 300);
  drawFor(ON, 300);
  drawFor(OFF, 300);
  drawFor(ON, 300);
  if (playerWinLast == LEFT) {
    drawFor(ARROW_LEFT, 1000);
  }
  else if (playerWinLast == RIGHT) {
    drawFor(ARROW_RIGHT, 1000);
  }

  // Dev function
  if(DEVMODE == true){
    Serial.print("Player win last ");
    Serial.println(playerWinLast);
    Serial.print("Player winner ");
    Serial.println(playerWinner);
    Serial.print("Player score master left ");
    Serial.println(playerScoreMaster[LEFT]);
    Serial.print("Player score master right ");
    Serial.println(playerScoreMaster[RIGHT]);
  }

  // If one of the players has reached three points, set the overall game state to win
  if ((playerScoreMaster[LEFT] == 3) || (playerScoreMaster[RIGHT] == 3)) {
    if (playerScoreMaster[LEFT] == 3) {
      playerWinner = LEFT;
    }
    else if ((playerScoreMaster[RIGHT] == 3)) {
      playerWinner = RIGHT;
    }
    STATE = win;
  }
  // But if no one wins, set the game state back to play and reset player round scores
  else {
    STATE = play;
    playerScore[LEFT] = playerScoreInit[LEFT];
    playerScore[RIGHT] = playerScoreInit[RIGHT];
  }
}

// If a player wins the entire game...
void playerWinGame() {

  // Flash the board
  drawFor(OFF, 300);
  drawFor(ON, 300);
  drawFor(OFF, 300);
  drawFor(ON, 300);

  // Draw their number if left
  if (playerWinner == LEFT) {
    drawFor(PLAYER_1, 1000);
    drawFor(PLAYER_1,  500, true);
    drawFor(PLAYER_1, 1000);
    drawFor(PLAYER_1,  500, true);
    drawFor(PLAYER_1, 1000);
    drawFor(PLAYER_1,  500, true);
    drawFor(PLAYER_1, 1000);
  }
  // or if right
  else if (playerWinner == RIGHT) {
    drawFor(PLAYER_2, 1000);
    drawFor(PLAYER_2,  500, true);
    drawFor(PLAYER_2, 1000);
    drawFor(PLAYER_2,  500, true);
    drawFor(PLAYER_2, 1000);
    drawFor(PLAYER_2,  500, true);
    drawFor(PLAYER_2, 1000);
  }
}

// Sometimes you have to stop the love
// Shorthand to stop playing the music
void stopMusic() {
  noTone(musicPin);
}

// A small function that updates the music speed based on the
// level of "winningness". More win equals more fast.
void updateMusicSpeed(int winLocation) {
  int absWinLocation = abs(winLocation - 4);
  if (absWinLocation < 2) {
    musicSpeed = 1; // normal speed
  }
  else if (absWinLocation < 3) {
    musicSpeed = 2; // kinda-fast
  }
  else {
    musicSpeed = 3; // real-fast
  }
}

// Function that plays the music
// Optionally, it can run "asyncronously"
// Syncronous function adapted from Melody example
void playMusic(int toneSpeed, bool async = false) {
  // No music if you've muted
  if (MUTE == true) {
    stopMusic();
  }
  else {
    // If you're doing asyncronous playing
    if (async == true) {
      /*
         Get masterclock time
         If masterclock time > lastnoteplayed + noteduration,
           play next note
         Otherwise keep playing the current note
      */

      masterClock = millis();
      noteDuration = (1000 / noteDurations[notePosition]) / toneSpeed;

      if (masterClock > lastNotePlayedClock + noteDuration) {
        if (music[notePosition] == 0) {
          noTone(musicPin);
        }
        else {
          tone(musicPin, music[notePosition]);
        }
        // Choose a random note to play
        notePosition = random(0, 7) % noteCount;
        lastNotePlayedClock = millis();
      }

    }
    // this is basically exactly what was in the Melody example
    else if (async == false) {
      // iterate over the notes of the melody:
      for (int thisNote = 0; thisNote < noteCount; thisNote++) {

        // to calculate the note duration, take one second
        // divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        noteDuration = (1000 / noteDurations[thisNote]) / toneSpeed;
        tone(musicPin, music[thisNote], noteDuration);

        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        double pauseBetweenNotes = noteDuration * 1;
        delay(pauseBetweenNotes);
        // stop the tone playing:
        noTone(musicPin);
      }
    }
  }
}

// To restart the game, you can hold both buttons down after someone wins
void restart() {
  updateBtnStates();
  if ((btnStates[LEFT] == HIGH) && (btnStates[LEFT] == HIGH)) {
    playerScore[LEFT] = playerScoreInit[LEFT];
    playerScore[RIGHT] = playerScoreInit[RIGHT];
    playerScoreMaster[LEFT] = 0;
    playerScoreMaster[RIGHT] = 0;
    STATE = startup;
  }

}
// The initial draw function, "loading"
void drawStartup() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      ledOn(i, j, true);
      delay(25);
    }
  }

  playMusic(1);
  playMusic(2);
  playMusic(1);


  STATE = play;
}

// The setup function. It sets up the environment on load.
void setup() {
  masterClock = millis();

  if (DEVMODE == true) {
    Serial.begin(9600);
  }

  // Set LEDs to output
  for (int thisLED = 0; thisLED < 8; thisLED++) {
    pinMode(ledPinsCols[thisLED], OUTPUT);
  };
  for (int thisLED = 0; thisLED < 8; thisLED++) {
    pinMode(ledPinsRows[thisLED], OUTPUT);
  };

  pinMode(musicPin, OUTPUT);

  // Set btns to input
  pinMode(btnPins[LEFT], INPUT);
  pinMode(btnPins[RIGHT], INPUT);

  if (DEVMODE == true) {
    // flash the board to make sure everything is plugged in
    setMatrixOn();
    delay(300);
    setMatrixOff();
    Serial.println("Setup complete");
  }

  // Set game state
  STATE = startup;
}



void loop() {
  // Master switch that controls what happens in which game state
  switch (STATE) {
    case startup:
      drawStartup();
      break;
    case play:
      updateScores();
      playMusic(musicSpeed, true);
      break;
    case playerWin:
      stopMusic();
      playerWinSingle();
      break;
    case win:
      stopMusic();
      playerWinGame();
      restart();
      break;
  }

}
