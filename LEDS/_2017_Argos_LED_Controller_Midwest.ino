

// 2017 Argos LED Controller i2c
// Receives Game Status from Robo RIO over i2c and controls LEDs

//Allow interupts with FastLED Library
#define FASTLED_ALLOW_INTERRUPTS 1


#include <FastLED.h>
#include <FastLED.h>
#include <Wire.h>
#include <SPI.h>

#define NUM_LEDS_SIDE 40// Number of LEDs on side of robot
#define NUM_LEDS_TUR 8// Number of LEDs on turrets
#define NUM_LEDS_FRONT 46//Number of LEDs on side of robot

// control the LEDs pins:
#define DATAPIN1    5// side leds
#define CLOCKPIN1   6 // side leds
#define DATAPIN2    8 // turret right LEDS
#define CLOCKPIN2   9 // turret right LEDS
#define DATAPIN3    4 // Front LEDS for gear placer status
#define CLOCKPIN3   3 // Front LEDS for gear placer status
#define DATAPIN4    10 // Turrent Left LEDS
#define CLOCKPIN4   11 // Turret Left LEDS

bool gReverseDirection = false;

//SET FAST LED Library settings for DotStar
#define COLOR_ORDER BGR
#define CHIPSET APA102
#define BRIGHTNESS  250
#define FRAMES_PER_SECOND 60


CRGB leds_side[NUM_LEDS_SIDE]; //setup array for side leds
CRGB leds_tur_left[NUM_LEDS_TUR]; //setup array for turret leds
CRGB leds_tur_right[NUM_LEDS_TUR];
CRGB leds_front[NUM_LEDS_FRONT]; //setup array for front leds for gear placer

//audio sample settings for disabled mode
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

//define variables for storing incoming i2c messages
char syncchar = 'X';
char gameMode = 'D';
char autonMode = 'S';
char gearPos = 'I';
char turretaim = 'U';
char win = 'N';
char alliance = 'R';
char climb = 'J';
unsigned char yoyoPos = 30;
unsigned char pegPos = 30;
char shoot = 'O';
int byteNum = 12;

//define global varriables to be used in cylon code for autonomous to allow multiple strips of different lengths
int led_side_cursor = 0;
int led_front_cursor = 0;
int led_turret_cursor = 0;
char dir_front = 'F';
char dir_side = 'F';
char dir_tur = 'F';

//FIRELED settings for win button
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  100

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 200

//setup for sound reactive lights
int p2p_average = 0;
int sound_scale = 512;
int lightDelay = 0;

//Color Definitions
uint32_t auto_gearright = CRGB::Green; //Green
uint32_t auto_gearleft = CRGB::Yellow; //Yellow
uint32_t auto_num5 = CRGB::Pink; //Pink
uint32_t auto_gearstraight = CRGB::White; //white
uint32_t auto_num6 = CRGB::Teal; //Teal
uint32_t auto_num4 = CRGB::Purple; //Purple
uint32_t turretColor = CRGB::Red;
uint32_t aimedColor = CRGB::Green;      // 'On' color (aimed = green)
uint32_t targetfoundColor = CRGB::Blue;      // 
uint32_t notargetColor = CRGB::Red; 
uint32_t alliancecolor = CRGB::Red;      // 'On' color (starts red)
uint32_t catyellowcolor = 0x444400;      
uint32_t autonModecolor = auto_gearstraight;      // auton color defaults to gear straight - white
uint32_t teleColor = alliancecolor; //Teleop color defaults to alliance color
uint32_t color = CRGB :: Black;
float yoyo_per = 50;
float peg_per =50;

void setup()
{
  Wire.begin(84);                // join i2c bus with address #84
  TWBR = 12;
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output

  FastLED.addLeds<CHIPSET, DATAPIN1,CLOCKPIN1, COLOR_ORDER>(leds_side, NUM_LEDS_SIDE);
  FastLED.addLeds<CHIPSET, DATAPIN2,CLOCKPIN2, COLOR_ORDER>(leds_tur_left, NUM_LEDS_TUR);
  FastLED.addLeds<CHIPSET, DATAPIN3,CLOCKPIN3, COLOR_ORDER>(leds_front, NUM_LEDS_FRONT);
  FastLED.addLeds<CHIPSET, DATAPIN4,CLOCKPIN4, COLOR_ORDER>(leds_tur_right, NUM_LEDS_FRONT);
  FastLED.setBrightness( BRIGHTNESS );

  Serial.print("ARGOS 1756!"); //message to indicate code is executing setup function

}

void loop()
{
    //troubleshooting code for outputting variables received from i2c bus 
     Serial.print(gameMode);
     Serial.print(autonMode);
     Serial.print(gearPos);
     Serial.print(win);
     Serial.print(alliance);
     Serial.print(turretaim); 
     Serial.print(climb);
     Serial.print(yoyoPos);
     Serial.print(pegPos);
     Serial.println(shoot);

//override all functions if win button is pressed on controller and run celebration mode function
  if (win == 'W') {
    celebrationMode();
  }
  else
  {
    switch (gameMode)
    {
      case 'A':
        auton();  //Auton mode light show
        break;
      case 'T':
        teleopMode(); //Teleop mode light show
        break;
      case 'D':
        disabledMode(); // Disabled Mode light show 
        break;
      default:
      ;
    }
  }
}

//celebration mode randomly assigns colors to leds

void FireSide()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS_SIDE];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS_SIDE; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS_SIDE) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS_SIDE - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = alliancecolor;
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS_SIDE; j++) {
      CRGB color = alliancecolor*heat[j];
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS_SIDE-1) - j;
      } else {
        pixelnumber = j;
      }
      leds_side[pixelnumber] = color;
    }
}



void celebrationMode()
{
  FireSide();

  //random colors for front and turrets
  for (int x=0; x<NUM_LEDS_TUR; x++)
  {
    color = random(); 
    leds_tur_left[x] = color;
    leds_tur_right[x] = color;
  }
  for (int x=0; x<NUM_LEDS_FRONT; x++)
  {
    color = random(); 
    leds_front[x] = color;
  } 

  FastLED.show();
  delay(1000/60);
}
// fade leds for cyclon effect in auton mode
void fadeall()
{ 
 
  for(int i = 0; i < NUM_LEDS_SIDE; i++) { leds_side[i].nscale8(220);}
  for(int i = 0; i < NUM_LEDS_FRONT; i++) { leds_front[i].nscale8(220);}
  for(int i = 0; i < NUM_LEDS_TUR; i++) { leds_tur_left[i].nscale8(100);}
  for(int i = 0; i < NUM_LEDS_TUR; i++) { leds_tur_right[i].nscale8(100);}
  } 

//led display for auton showing cyclon effect  
void auton()
{ 
  while (gameMode == 'A') {
  if ((led_side_cursor < NUM_LEDS_SIDE) && (dir_side == 'F'))
  {
     led_side_cursor++;
  }
  else if (led_side_cursor == NUM_LEDS_SIDE)
  {
     dir_side = 'B';
     led_side_cursor = NUM_LEDS_SIDE -1;
  }
  else
   {
      if (led_side_cursor == 0)
       {
        dir_side = 'F';
       }
       else
       {
        led_side_cursor--;
       }
   }


  if ((led_front_cursor < NUM_LEDS_FRONT) && (dir_front == 'F'))
  {
     led_front_cursor++;
  }
  else if ( (led_front_cursor < NUM_LEDS_FRONT) && (dir_front == 'B'))
   {
      if (led_front_cursor == 0)
       {
        dir_front = 'F';
       }
       else
       {
        led_front_cursor--;
       }
   }
  else
  {
    dir_front = 'B';
    led_front_cursor = NUM_LEDS_FRONT -1;
  }
  
  if ((led_turret_cursor < NUM_LEDS_TUR) && (dir_tur == 'F'))
  {
     led_turret_cursor++;
  }
   
  else if (( led_turret_cursor < NUM_LEDS_TUR )&& (dir_tur == 'B'))
   {
      if (led_turret_cursor == 0)
       {
        dir_tur = 'F';
       }
       else
       {
        led_turret_cursor--;
       }
   }
  
  else
  {
     dir_tur = 'B';
     led_turret_cursor = NUM_LEDS_TUR -1;
  }
    leds_side[led_side_cursor] = alliancecolor;
    leds_tur_left[led_turret_cursor] = alliancecolor;
    leds_tur_right[led_turret_cursor] = alliancecolor;
    leds_front[led_front_cursor] = alliancecolor;
    // Show the leds
    FastLED.show(); 
    fadeall();
    delay(12);
  }
}

void teleopMode()
{
  int yoyo = 0;
  int peg = 0;
  
   for (int x=0; x<NUM_LEDS_TUR; x++)
   {
     leds_tur_left[x] = turretColor;
     leds_tur_right[x] = turretColor;
   }
   for (int x=0; x<NUM_LEDS_SIDE; x++)
   {
    leds_side[x] = alliancecolor;
   } 
   for (int x=0; x<NUM_LEDS_FRONT; x++)
   {
    switch (gearPos)
    {
      case 'C':
        leds_front[x] = catyellowcolor;
        break;
      case 'P':
        leds_front[x] = CRGB::Green;
        peg = (peg_per * NUM_LEDS_FRONT);
        leds_front[peg] = CRGB::White;
        yoyo = yoyo_per * NUM_LEDS_FRONT;
        leds_front[yoyo] = CRGB::White;
        break;
       case 'H':
        leds_front[x] = CRGB::Purple;
        break;
        default:
           leds_front[x] = alliancecolor;
           break;     
    }
    
  }
  FastLED.show();

}

// Disable Mode Robot code

void disabledMode()
{
  for (int i = 0; i < NUM_LEDS_TUR; i++)
  {
   leds_tur_left[i] = autonModecolor;
  }  
   
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level
   int i,j;
   unsigned int signalMax = 0;
   unsigned int signalMin = 600;
 
   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(0);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
      if (gameMode != 'D') {
      break;
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   p2p_average = (peakToPeak + p2p_average) / 2;
   if (p2p_average > 450)
   {
    sound_scale = 612;
   }
   else if (p2p_average <= 450 && p2p_average > 100)
   {
    sound_scale = 512;
   }
   //else if (p2p_average <= 250 && p2p_average > 100)
   //{
   // sound_scale = 412;
   // }
    else
      sound_scale = 100;
   

   int height_side = (peakToPeak * NUM_LEDS_SIDE/2) / sound_scale ; // convert to volts
   int height_front = (peakToPeak * NUM_LEDS_FRONT/2) / sound_scale ; // convert to volts
   int height_tur = (peakToPeak * NUM_LEDS_TUR) / sound_scale ; // convert to volts
 
   for(i=NUM_LEDS_SIDE/2; i>=0; i--) {
    if(i < NUM_LEDS_SIDE/2 - height_side)
    {
      leds_side[i] = CRGB::Black;
      leds_side[NUM_LEDS_SIDE-i] = CRGB::Black;
    }
    else
    {
      leds_side[i] = alliancecolor;
      leds_side[NUM_LEDS_SIDE-i] = alliancecolor;
    }    
  }
  for(i=NUM_LEDS_FRONT/2; i>=0; i--) {
    if(i < NUM_LEDS_FRONT/2 - height_front)
    {
      leds_front[i] = CRGB::Black;
      leds_front[NUM_LEDS_FRONT-i] = CRGB::Black;
    }
    else
    {
      leds_front[i] = alliancecolor;
      leds_front[NUM_LEDS_FRONT-i] = alliancecolor;
      
    }    
  }
  for(i=0; i<NUM_LEDS_TUR; i++) {
    if(i <= height_tur)
    {
      leds_tur_left[i] = autonModecolor;
      leds_tur_right[i] = autonModecolor;
    }
    else
    {
      leds_tur_left[i] = CRGB::Black;
      leds_tur_right[i] = CRGB::Black;
    }    
  }
  FastLED.show(); // Update strip

  //troubleshooting for sound reactive lights
  // Serial.print(height);
  // Serial.print(" , ");
  //Serial.println(peakToPeak);
  //Serial.print(" , ");
  //Serial.println(sound_scale);
}


// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int numBytes)
{
 
  char text[byteNum + 1];
  //char blank[byteNum + 1];
  int i = 0;
  while (Wire.available()) {
    text[i] = Wire.read();
    text[i + 1] = '\0';
    i++;
  }
  //Save message if sync Char is equal to M
  if (syncchar == 'M')
  {
  
 //Store characters received via i2c into variables
  
  syncchar = text[0];
  gameMode = text[1]; // receive byte as a character
  autonMode = text[2];
  gearPos = text[3];
  win = text[4];
  alliance = text[5];
  turretaim = text[6];
  climb = text[7];
  yoyoPos= text[8];
  pegPos = text[9];
  shoot = text [10];

  //convert ultrasonic sensor and yoyo into a percent to be multipled by # of pixels
  yoyo_per = (float)yoyoPos / (float)99;
  peg_per = (float)pegPos / (float)99;

 //Set Alliance Color
  if (alliance == 'R')
  {
    alliancecolor = CRGB::Red;
  }
  else
  {
    alliancecolor = CRGB::Blue;
  }
  //Set Turrent Color based on status
  if (turretaim == 'Q')
  {
    turretColor = aimedColor;
  }
  else if (turretaim == 'Y')
  {
    turretColor = targetfoundColor;
  }
  else
  {
    turretColor = notargetColor;
  }
  //set auto color to be displayed in disabled mode
  switch (autonMode)
    {
      case 'L':
        autonModecolor = auto_gearleft;
        break;
      case 'G':
        autonModecolor = auto_gearright;
        break;
      case 'F':
        autonModecolor = auto_num4;
        break;
      case 'K':
        autonModecolor = auto_num5;
        break; 
       case 'V':
        autonModecolor = auto_num6;
        break;                 
      default:
        autonModecolor = auto_gearstraight;
    }
  }
}
