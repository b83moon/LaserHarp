#include <Metro.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <AltSoftSerial.h>    // Arduino build environment requires this
#include <wavTrigger.h>

/* WAV trigger */
#define LED 13                // our LED
#define LASER_PIN 49          // Laser PIN
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




void setup(void) {
  Serial.begin(9600);
  Serial.println("Hello!");
  /* WAV trigger setup */
  // Initialize the LED pin
  pinMode(LED,OUTPUT);
  pinMode(LASER_PIN,OUTPUT);
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
}


/******************* MAIN LOOP ****************/
void loop(void) {
   playHarp();
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

//This function checks if a note has been struck and returns a boolean
boolean checkIfNoteStruck() {
  harpString.buttonState = digitalRead(harpString.button);
  if (harpString.buttonState == LOW) {
    Serial.println("Note struck.");
    return true;
  } else {
    return false;
  }
}


void goToPosition(int position, int VA, int VB, int num_positions) {
  //Turn off laser before moving
  turnLaserOff();
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
  if(lasers == 1) {
    lasers = 2;
  }
  uint32_t segment = range/(lasers - 1);
  uint32_t position_var = (uint32_t)position;
  uint32_t output_voltage = VA_op + (position_var - 1)*segment;
  dac.setVoltage(output_voltage,false);
  //Turn laser back on
  turnLaserOn();
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


void turnLaserOff() {
  digitalWrite(LASER_PIN,HIGH);
}

void turnLaserOn() {
  digitalWrite(LASER_PIN,HIGH);
}







