

// 2017 Argos LED Controller i2c
// Receives Game Status from Robo RIO over i2c and controls LEDs

#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>


#include <FastLED.h>
#include <Wire.h>
//#include <Adafruit_DotStar.h>
#include <SPI.h>

#define NUM_LEDS_SIDE 30// Number of LEDs on side of robot
#define NUM_LEDS_TUR 15// Number of LEDs on turrets

// control the LEDs pins:
#define DATAPIN1    4 // side leds
#define CLOCKPIN1   5 // side leds
#define DATAPIN2    8 // turret leds
#define CLOCKPIN2   9 // turret leds
#define COLOR_ORDER GBR
#define CHIPSET APA102
#define BRIGHTNESS  250
#define FRAMES_PER_SECOND 60


CRGB leds_side[NUM_LEDS_SIDE]; //setup array for side leds
CRGB leds_tur[NUM_LEDS_TUR]; //setup array for turret leds


const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
char gameMode = 'D';
char autonMode = 'S';
char gearPos = 'I';
char turretaim = 'U';
char win = 'N';
char alliance = 'R';
char climb = 'J';
unsigned char yoyoPos = 30;
unsigned char pegPos = 30;

int head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
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
uint32_t targetfoundColor = CRGB::Blue;      // 'On' color (not aimed = purple)
uint32_t notargetColor = CRGB::Red; 



uint32_t alliancecolor = 0xFF0000;      // 'On' color (starts red)
uint32_t disablecolor = 0xFF0000;      // 'On' color (starts red)
uint32_t catyellowcolor = 0x444400;      
uint32_t autonModecolor = 0x444400;      // 'On' color (starts CAT Yellow)
uint32_t color = 0xFF0000;
uint32_t teleColor = alliancecolor;


void setup()
{
  Wire.begin(84);                // join i2c bus with address #84
  TWBR = 12;
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output

  FastLED.addLeds<CHIPSET, DATAPIN1,CLOCKPIN1, COLOR_ORDER>(leds_side, NUM_LEDS_SIDE).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<CHIPSET, DATAPIN2,CLOCKPIN2, COLOR_ORDER>(leds_tur, NUM_LEDS_TUR).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  Serial.print("ARGOS 1756!");

}

void loop()
{

  //   Serial.print(gameMode);
  //   Serial.print(gameTime);
  //   Serial.print(battLevel);
  //   Serial.print(yoyoPos);
  //   Serial.print(win);
  //   Serial.println(alliance);
  //Serial.println("main");


  if (win == 'W') {
    celebrationMode();
  }
  else
  {
    switch (gameMode)
    {
      case 'A':
        auton();  //Auto Blue
        break;
      case 'T':
        teleopMode(); //Tele Green
        break;
      default:
        disabledMode(); // Disabled Red
    }

  }

  //  strip.show();
}

void celebrationMode()
{
  for (int x = 0; x < NUM_LEDS_SIDE; x++)
  {
    color = random();
    leds_side[x] = color;
  }
  FastLED.show();
  delay(50);
}

void auton()
{
 
  for (int x = 0; x < NUM_LEDS_SIDE; x++)
  {

   leds_side[x] = alliancecolor;
  }
  while (gameMode == 'A')
  {
  
  for (int x = tail; x < head; x++)
  {
    leds_side[x] = CRGB::Green;
  }
    leds_side[tail-1] = alliancecolor;
  FastLED.show();
  if (tail <= NUM_LEDS_SIDE)
  {
    head++;
    tail++;
  }
  else
  {
    head=0;
    tail=-10;
  }
  }


}

void teleopMode()
{
    
  for (int x = 0; x < NUM_LEDS_SIDE; x++)
  {

    leds_side[x] = teleColor;
    
  }
  FastLED.show();
  delay(lightDelay);
 
  if (lightDelay >0)
  {
    for (int x = 0; x < NUM_LEDS_SIDE; x++)
    {
  
      leds_side[x] = CRGB::Black;

    }
    FastLED.show();
    delay(lightDelay);
  }
  for (int x = 0; x < NUM_LEDS_SIDE; x++)
  {

    leds_side[x] = teleColor;
  }
  FastLED.show();

}

void disabledMode()
{
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
   

   int height = (peakToPeak * NUM_LEDS_SIDE/2) / sound_scale ; // convert to volts
 
   for(i=NUM_LEDS_SIDE/2; i>=0; i--) {
    if(i < NUM_LEDS_SIDE/2 - height)
    {
      leds_side[i] = CRGB::Black;
      leds_side[NUM_LEDS_SIDE-i] = CRGB::Black;
    }
    else
    {
      leds_side[i] = autonModecolor;
      leds_side[NUM_LEDS_SIDE-i] = autonModecolor;
    }
  }
  FastLED.show(); // Update strip

   Serial.print(height);
   Serial.print(" , ");
   Serial.println(peakToPeak);
   Serial.print(" , ");
   Serial.println(sound_scale);

  
}


// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int numBytes)
{
  String temp = "";
  int byteNum = 10;
  char text[byteNum + 1];
  char blank[byteNum + 1];
  int i = 0;
  while (Wire.available()) {
    text[i] = Wire.read();
    //         Serial.print(text[i]);
    text[i + 1] = '\0';
    i++;
  }
  // Serial.println('\0');
  gameMode = text[1]; // receive byte as a character
  //   Serial.print(gameMode);         // print the character
  autonMode = text[2];
  //   Serial.print(autonMode);
  gearPos = text[3];
  //    Serial.println(gearPos);
  win = text[4];
  //   Serial.print(win);
    alliance = text[5];
  //   Serial.println(alliance);
    turretaim = text[6];
  //   Serial.println(turret); 
    climb = text[7];
  //   Serial.println(climb);
      yoyoPos= text[8];
  //   Serial.println(yoyoPos);
      pegPos = text[9];
  //   Serial.println(pegPos);
 

  if (alliance == 'R')
  {
    alliancecolor = 0xFF0000;
  }
  else
  {
    alliancecolor = 0x0000FF;
  }
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
