/* 
Lighting Cloud Mood Lamp By James Bruce
View the full tutorial and build guide at http://www.makeuseof.com/

Sound sampling code originally by Adafruit Industries.  Distributed under the BSD license.
This paragraph must be included in any redistribution.
*/


/* Modifications par Yvan Gimard
 *  DUT - MMI2 2019
  */

#include <Wire.h>
#include "FastLED.h"

// Ruban LED
#define NUM_LEDS 25 //Changer ce nombre selon la quantité de LEDs dans le ruban
#define DATA_PIN 6  //Connecter le câble du milieu "din" (change selon le modèle) sur le port digital 6 de l'arduino

//Capteur ultrason
int trigPin = 9;    // Trigger
int echoPin = 10;    // Echo
long duration, cm;
int validDist;

const int butt2 = 10;
 int counter = 90; 
 int aState;
 int aLastState; 
bool lightning = true;

int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;    // the debounce time; increase if t


// Mode enumeration - if you want to add additional party or colour modes, add them here; you'll need to map some IR codes to them later; 
// and add the modes into the main switch loop
enum Mode { CLOUD,ACID,OFF,ON,RED,GREEN,BLUE,FADE};
Mode mode = CLOUD;  
Mode lastMode = CLOUD;

// Mic settings, shouldn't need to adjust these. 
#define MIC_PIN   0  // Microphone is attached to this analog pin
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   10  // Length of buffer for dynamic level adjustment
byte
  volCount  = 0;      // Frame counter for storing past volume data
int
  vol[SAMPLES];       // Collection of prior volume samples
int      n, total = 30;
float average = 0;
  
// used to make basic mood lamp colour fading feature
int fade_h;
int fade_direction = 1;


// Define the array of leds
CRGB leds[NUM_LEDS];



void setup() { 
  // this line sets the LED strip type - refer fastLED documeantion for more details https://github.com/FastLED/FastLED
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  // starts the audio samples array at volume 15. 
  memset(vol, 15, sizeof(vol));
  Serial.begin(9600);
//  pinMode(butt,INPUT_PULLUP);
//   pinMode (outputA,INPUT);
//   pinMode (outputB,INPUT);
  Wire.begin(9);                // Start I2C Bus as a Slave (Device Number 9)
  Wire.onReceive(receiveEvent); // register event
//  aLastState = digitalRead(outputA); 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); 
}



void receiveEvent(int bytes) {
  
  // Here, we set the mode based on the IR signal received. Check the debug log when you press a button on your remote, and 
  // add the hex code here (you need 0x prior to each command to indicate it's a hex value)
  while(Wire.available())
   { 
      unsigned int received = Wire.read();
      Serial.print("Receiving IR hex: ");
      Serial.println(received,HEX);
      lastMode = mode;
      switch(received){
        case 0x3F:
          mode = ON; break;
        case 0xBF:
          mode = OFF; break;
        case 0x2F:
          mode = CLOUD; break;
        case 0xF:
          mode = ACID; break;
        case 0x37:
          mode = FADE; break;
        case 0x9F:
          mode = BLUE; break;
        case 0x5F:
          mode = GREEN; break;
        case 0xDF:
          mode = RED; break;
        
      }
   }

}
 
void loop() {  

   if (lightning) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(500);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);   
    pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH);   
    // Convert the time into a distance
    cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
    if ( cm > 2000) {
      cm = validDist;
    }
    validDist = cm;
    aLastState = aState;
    if ( cm < 30) {
      int counterMap = map (cm, 30, 0, 1200, 20); //Convertir la distance en un nombre servant pour le delay entre deux éclairs
      constant_lightning(counterMap);         //Appel de la fonction d'éclair avec notre délai en paramètre
      reset();
      delay(random(50,100));
    }    
   } else if (!lightning) {
    
   }
}

// Makes all the LEDs a single colour, see https://raw.githubusercontent.com/FastLED/FastLED/gh-pages/images/HSV-rainbow-with-desc.jpg for H values
void single_colour(int H){
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = CHSV( H, 255, 255);
  }
  //avoid flickr which occurs when FastLED.show() is called - only call if the colour changes
  if(lastMode != mode){
    FastLED.show(); 
    lastMode = mode;
  }
  delay(50);
}

void colour_fade(){
  //mood mood lamp that cycles through colours
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = CHSV( fade_h, 255, 255);
  }
  if(fade_h >254){
    fade_direction = -1; //reverse once we get to 254
  }
  else if(fade_h < 0){
    fade_direction = 1;
  }
    
  fade_h += fade_direction;
  FastLED.show();
  delay(100);
}



void detect_thunder() {
  
  n   = analogRead(MIC_PIN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  vol[volCount] = n;                      // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  total = 0;
  for(int i=0; i<SAMPLES; i++) {
    total += vol[i];
  }
  
  average = (total/SAMPLES)+2;
  if(n>average){
    Serial.println("TRIGGERED");
    reset();
     
   
    //I've programmed 3 types of lightning. Each cycle, we pick a random one. 
    switch(random(1,3)){
       //switch(3){
  
      case 1:
        thunderburst();
        delay(random(10,500));
         Serial.println("Thunderburst");
        break;
       
      case 2:
        rolling();
        Serial.println("Rolling");
        break;
        
      case 3:
        crack();
        delay(random(50,250));
        Serial.println("Crack");
        break;
        
      
    }
  }
}
 
 
// utility function to turn all the lights off.  
void reset(){
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = CHSV( 0, 0, 0);
  }
  FastLED.show();
   
}

void acid_cloud(){
    // a modification of the rolling lightning which adds random colour. trippy. 
    //iterate through every LED
    for(int i=0;i<NUM_LEDS;i++){
      if(random(0,100)>90){
        leds[i] = CHSV( random(0,255), random(0,255), random(0,255)); 

      }
      else{
        leds[i] = CHSV(0,0,0);
      }
    }
    FastLED.show();
    delay(random(5,100));
    reset();
    
  //}
}

void rolling(){
  // a simple method where we go through every LED with 1/10 chance
  // of being turned on, up to 10 times, with a random delay wbetween each time
  for(int r=0;r<random(2,10);r++){
    //iterate through every LED
    for(int i=0;i<NUM_LEDS;i++){
      if(random(0,100)>90){
        leds[i] = CHSV( 0, 0, 255); 

      }
      else{
        //dont need reset as we're blacking out other LEDs her 
        leds[i] = CHSV(0,0,0);
      }
    }
    FastLED.show();
    delay(random(5,100));
    reset();
    
  }
}

void crack(){
   //turn everything white briefly
   for(int i=0;i<NUM_LEDS;i++) {
      leds[i] = CHSV( 0, 0, 255);  
   }
   FastLED.show();
   delay(random(10,100));
   reset();
}

void thunderburst(){

  // this thunder works by lighting two random lengths
  // of the strand from 10-20 pixels. 
  int rs1 = random(0,NUM_LEDS/2);
  int rl1 = random(10,20);
  int rs2 = random(rs1+rl1,NUM_LEDS);
  int rl2 = random(10,20);
  
  //repeat this chosen strands a few times, adds a bit of realism
  for(int r = 0;r<random(3,6);r++){
    
    for(int i=0;i< rl1; i++){
      leds[i+rs1] = CHSV( 0, 0, 255);
    }
    
    if(rs2+rl2 < NUM_LEDS){
      for(int i=0;i< rl2; i++){
        leds[i+rs2] = CHSV( 0, 0, 255);
      }
    }
    
    FastLED.show();
    //stay illuminated for a set time
    delay(random(10,50));
    
    reset();
    delay(random(10,50));
  }
  
}

// basically just a debug mode to show off the lightning in all its glory, no sound reactivity. 
//Le mode constant, à lancer quand on souhaite juste que le nuage fasse du tonnerre aléatoirement
void constant_lightning(int counterMapP){
//  switch(random(1,counterMapP)){  //On génère un nombre aléatoire entre 1 et 10. On peut augmenter la plage de nombre si l'on souhaite avoir moins d'éclairs
//   case 1:              //Si 1, effectuer cela. Ça permet d'avoir 10% de chance de tomber sur cette fonction
//        thunderburst();
//        delay(random(10,50));  //Un delay random pour mettre de la variation
//         Serial.println("Thunderburst");
//        break;
//       
//      case 2:
//        rolling();
//        Serial.println("Rolling");
//        break;
//        
//      case 3:
//        crack();
//        delay(random(50,150));
//        Serial.println("Crack");
//        break;
//        
//    
//  }  
  switch(random(1,3)){  //On génère un nombre aléatoire entre 1 et 3. On peut augmenter la plage de nombre si l'on souhaite avoir moins d'éclairs
   case 1:              //Si 1, effectuer cela. Ça permet d'avoir 33% de chance de tomber sur cette fonction
        thunderburst();
        delay(random(10,250));  //Un delay random pour mettre de la variation
         Serial.println("Thunderburst");
        break;
       
      case 2:
        rolling();
        Serial.println("Rolling");
        break;
        
      case 3:
        crack();
        delay(random(50,150));
        Serial.println("Crack");
        break;        
  }  
  delay(counterMapP); //On utilise le paramètre pour attendre avant le prochain éclair
}
  
