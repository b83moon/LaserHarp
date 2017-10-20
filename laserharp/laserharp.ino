#include "Adafruit_RA8875.h"

#include <Metro.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <AltSoftSerial.h>    // Arduino build environment requires this
#include <wavTrigger.h>

#include <SPI.h>
#include "Adafruit_GFX.h"

/* WAV trigger */
#define LED 13                // our LED

wavTrigger wTrig;             // Our WAV Trigger object

// NOT NEEDED? vvvvv
Metro gLedMetro(500);         // LED blink interval timer
Metro gWTrigMetro(6000);      // WAV Trigger state machine interval timer

byte gLedState = 0;           // LED State
int  gWTrigState = 0;         // WAV Trigger state
int  gRateOffset = 0;         // WAV Trigger sample-rate offset
// NOT NEEDED? ^^^^^

/* BUTTON acting as harp string (laser position) */
struct Button {
  bool isNotePlaying;
  bool isNotePressed;
  int track;  //track #
  int button; // pin
  int buttonState;  //low/high
};

Button harpString;

/* DAC */
int counter_int = 1;
int updown = 1;
Adafruit_MCP4725 dac;
void goToPosition(int position, int VA, int VB, int num_positions);

int laser = 40;

bool isNoteStruck;


/* TFT menu screen */
// Library only supports hardware SPI at this time
// Connect SCLK to MEGA Digital #52 (Hardware SPI clock)
// Connect MISO to MEGA Digital #50 (Hardware SPI MISO)
// Connect MOSI to MEGA Digital #51 (Hardware SPI MOSI)
#define RA8875_INT 3
#define RA8875_CS 10
#define RA8875_RESET 9

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
uint16_t tx, ty;

int16_t screenWidth;
int16_t screenHeight;

void setup(void) {
  Serial.begin(9600);
  Serial.println("Hello!");

  /* WAV trigger setup */
  // Initialize the LED pin
  pinMode(LED,OUTPUT);
  digitalWrite(LED,gLedState);

  // WAV Trigger startup at 57600
  wTrig.start();
  
  // If the Uno is powering the WAV Trigger, we should wait for the WAV Trigger
  //  to finish reset before trying to send commands.
  delay(1000);
  
  // If we're not powering the WAV Trigger, send a stop-all command in case it
  //  was already playing tracks. If we are powering the WAV Trigger, it doesn't
  //  hurt to do this.
  wTrig.stopAllTracks();

  // button setup
  harpString.button = 7;
  harpString.buttonState = 0;

  // start
  Serial.print("Start up note");
  playNote(7);
  delay(3000);
  stopNote(7);
  Serial.print("End start up note");

  /* DAC setup */
  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  dac.begin(0x62);

  pinMode(laser, OUTPUT);
  digitalWrite(laser, HIGH);
    
  Serial.println("Generating a triangle wave");


  /* playHarp*/
  int position = 0;

  /* tft menu screen */
  Serial.println("RA8875 start");

  /* Initialise the display using 'RA8875_480x272' or 'RA8875_800x480' */
  if (!tft.begin(RA8875_800x480)) {
    Serial.println("RA8875 Not Found!");
    while (1);
  }
  
  Serial.println("Found RA8875.");

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.fillScreen(RA8875_BLUE);
  delay(1500);

  /* Set screen width and height variables */
  screenWidth = tft.width();
  screenHeight = tft.height();

  /* Switch to text mode */  
  tft.textMode();
  startMenu();
  /* this code cycles through all possible menus for 3 seconds each
  delay(3000);

  clearScreen();

  settingsMenu();
  delay(3000);
  clearScreen();

  setMenuBackground(5);
  delay(3000);
  clearScreen();
  
  PlayModeMessage();
  delay(4000);
  */
}

void playNote(int note) {
  // note is an integer corresponding to the track number
  wTrig.masterGain(0);                  // Reset the master gain to 0dB                 
  wTrig.trackPlayPoly(note);               // Start Track 
  Serial.println("Playing...");
}

void stopNote(int note) {
  // note is an integer corresponding to the track number
  wTrig.trackStop(note);
  Serial.println("Stopped playing...");
}

void loop(void) {
   playHarp();
}

void goToPosition(int position, int VA, int VB, int num_positions) {

  //num_positions = amount of states (8 = 8 laser positions)
  //VA = Low voltage, VB = High voltage. va vb = 3 5 = range is between 3-5 volts
  //position = position to turn to. 
  // if num_positions = 8, position = 3, VA = 3, VB = 5, then 
  //the function will write the DAC to Voltage 3 + (2V*pos3/8positions) 
  //which equals voltage 3.75v
  uint32_t VA_op = VA*4096/5;
  uint32_t VB_op = VB*4096/5;
  uint32_t VC_op = VB_op - VA_op;

  uint32_t range = VC_op;
  uint32_t lasers = num_positions;
  if(lasers == 1)
  {lasers = 2;
  }
  uint32_t segment = range/(lasers - 1);
  uint32_t position_var = (uint32_t)position;
  uint32_t output_voltage = VA_op + (position_var - 1)*segment;
  dac.setVoltage(output_voltage,false);
}


// This Function plays the laser harp
void playHarp(){
  int numberOfNotesPlaying = 0;
  bool wasNoteStruck[4] = {false,false,false, false};

  int startingPosition = 1;
  int position = 1;
  int numberOfPositions = 4;
  goToPosition(position,0,5,numberOfPositions);

  while(numberOfNotesPlaying < 3) {
      // Check to see if the note has been struck
      isNoteStruck = checkIfNoteStruck();
      Serial.print("Position:");
      Serial.println(position);

      // if a new note has been struck play the note
      // if a note has stopped being played, stop that note
      if(isNoteStruck && wasNoteStruck[position-1] == false){
        // a new note has been selected so start playing that note
        startNote(position);
        numberOfNotesPlaying++; //increase the total number of notes being played
        wasNoteStruck[position-1] = true; // set wasNotePlaying true for next time around
      } else if(!isNoteStruck && wasNoteStruck[position-1]) { 
        // a note that was playing is no longer being played 
        endNote(position); //stop that note
        numberOfNotesPlaying--; // decrease the total number of notes playing
        wasNoteStruck[position-1] = false; //set wasNotePlaying false for next time around
      }

      // move to next position, if position is at the end, reset back to starting position
      if(position >= numberOfPositions) {
        position = 1;
      } else {
        position++;
      }
      goToPosition(position,0,5,numberOfPositions);
      delay(2500);
  }
}


// This function takes in a given note and begins to play it
int startNote(int note) {
  // note is an integer corresponding to the track number of the note
  wTrig.masterGain(0);                     // Reset the master gain to 0dB                 
  wTrig.trackPlayPoly(note);               // Start Track 
  Serial.print("Playing track ");
  Serial.println(note);
}


//This function takes in a given note and stops playing it
int endNote(int note) {
  // note is an integer corresponding to the track number
  wTrig.trackStop(note);
  Serial.print("Stopped playing track ");
  Serial.println(note);
}

boolean checkIfNoteStruck() {
  harpString.buttonState = digitalRead(harpString.button);
  if (harpString.buttonState == LOW) {
    Serial.println("Note struck.");
    return true;
  } else {
    return false;
  }
}

void setMenuBackground(int numberOfColumns) {
  /* This function draws the background of the menu according to the numberOfColumns i.e number of choices for that menu */
  switch (numberOfColumns) {
    case 1:
      tft.fillScreen(RA8875_BLACK);
      break;
    case 2:
      tft.fillRect(0,0, screenWidth/2, screenHeight, RA8875_YELLOW);
      tft.fillRect((screenWidth/2)+1,0, screenWidth/2, screenHeight, RA8875_CYAN);
      break;
    case 3:
      tft.fillRect(0,0, screenWidth/3, screenHeight, RA8875_YELLOW);
      tft.fillRect((screenWidth/3)+1,0, screenWidth/3, screenHeight, RA8875_CYAN);
      tft.fillRect((screenWidth/3)*2+1,0, screenWidth/3, screenHeight, RA8875_BLUE);
      break;
    case 4:
      tft.fillRect(0,0, screenWidth/4, screenHeight, RA8875_YELLOW);
      tft.fillRect((screenWidth/4)+1,0, screenWidth/4, screenHeight, RA8875_CYAN);
      tft.fillRect((screenWidth/4)*2+2,0, screenWidth/4, screenHeight, RA8875_BLUE);
      tft.fillRect((screenWidth/4)*3+3, 0, screenWidth/4, screenHeight, RA8875_RED);
      break;
    case 5:
      tft.fillRect(0,0, screenWidth/5, screenHeight, RA8875_YELLOW);
      tft.fillRect((screenWidth/5)+1,0, screenWidth/5, screenHeight, RA8875_CYAN);
      tft.fillRect((screenWidth/5)*2+2,0, screenWidth/5, screenHeight, RA8875_BLUE);
      tft.fillRect((screenWidth/5)*3+3, 0, screenWidth/5, screenHeight, RA8875_RED);
      tft.fillRect((screenWidth/5)*4+4, 0, screenWidth/5, screenHeight, RA8875_BLACK);
      break;
  }
}


void startMenu() {
  // Draws the starting menu (play harp, settings)
  setMenuBackground(2);
  tft.textTransparent(RA8875_BLACK);
  tft.textEnlarge(4);

  // Write PLAY HARP
  char playHarpString[10] = "PLAY HARP";
  tft.textSetCursor(50,(screenHeight/2)-25);
  tft.textWrite(playHarpString);

  // Write SETTINGS
  char settingsString[9] = "SETTINGS";
  tft.textSetCursor(screenWidth/2 + 70,(screenHeight/2)-25);
  tft.textWrite(settingsString);
}

void settingsMenu() {
  // Draws the settings menu (background beats, notes selection, back)
  setMenuBackground(3);
  tft.textTransparent(RA8875_BLACK);
  tft.textEnlarge(2);

  // Write Background Beats (two lines)
  char backgroundString[11] = "Background";
  tft.textSetCursor(13,(screenHeight/2)-50);
  tft.textWrite(backgroundString);
  char beatsString[6] = "Beats";
  tft.textSetCursor(75,(screenHeight/2));
  tft.textWrite(beatsString);

  // Write Notes Selection (two lines)
  char notesString[6] = "Notes";
  tft.textSetCursor(screenWidth/3+70,(screenHeight/2)-50);
  tft.textWrite(notesString);
  char selectionString[10] = "Selection";
  tft.textSetCursor(screenWidth/3+30,(screenHeight/2));
  tft.textWrite(selectionString);

  // Write BACK
  tft.textEnlarge(3);
  char backString[5] = "BACK";
  tft.textSetCursor((screenWidth/3)*2+80,(screenHeight/2)-30);
  tft.textWrite(backString);
}

void PlayModeMessage() {
  setMenuBackground(1);
  tft.textTransparent(RA8875_WHITE);
  tft.textEnlarge(3);

  tft.textSetCursor(50, screenHeight/3);
  tft.textWrite("CURRENTLY IN PLAY MODE");

  tft.textSetCursor(60, 250);
  tft.textEnlarge(2);
  tft.textWrite("Play 4 strings simultaneously      to go back to SETTINGS.");
}

void clearScreen() {
  tft.fillScreen(RA8875_WHITE);
}




