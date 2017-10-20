


#define analogPin1 1     // potentiometer wiper (middle terminal) connected to analog pin 
#define analogPin2 2     // potentiometer wiper (middle terminal) connected to analog pin 3
#define analogPin3 3     // potentiometer wiper (middle terminal) connected to analog pin 3
#define laserPin 31
                       // outside leads to ground and +5V

int photodiode1 = 0;           // variable to store the value read
int photodiode2 = 0;           // variable to store the value read
int photodiode3 = 0;           // variable to store the value read
int photodiodeCalibrationAverage = 0;
int photodiodeAverage = 0;
int thresholdValue = 5;
boolean isStruck = false;

void setup() {
  Serial.begin(9600);          //  setup serial
  pinMode(31,OUTPUT);
}



void loop() {
  isStruck = isNoteStruck();
  Serial.println(isStruck);
}


boolean isNoteStruck() {
  
  LaserOff();
  Serial.println("laser Off calibration");
  delay(3000);
  photodiode1 = analogRead(analogPin1);
  photodiode2 = analogRead(analogPin2);   
  photodiode3 = analogRead(analogPin3);   
  photodiodeCalibrationAverage = (photodiode1 + photodiode2 + photodiode3)/3;
  Serial.println(photodiodeCalibrationAverage);
  
  LaserOn();
  Serial.println("laser On calibration");
  delay(3000);
  photodiode1 = analogRead(analogPin1);
  photodiode2 = analogRead(analogPin2);   
  photodiode3 = analogRead(analogPin3);   
  photodiodeCalibrationAverage = (photodiode1 + photodiode2 + photodiode3)/3; 
    Serial.println(photodiodeCalibrationAverage);

  if(photodiodeAverage > (photodiodeCalibrationAverage + thresholdValue)) {
    return true;
  }
  else {
    return false;
  }
}

void LaserOff() {
  digitalWrite(laserPin,LOW);
}

void LaserOn() {
  digitalWrite(laserPin,HIGH);
}
