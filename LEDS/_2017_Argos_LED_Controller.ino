

// 2017 Argos LED Controller i2c
// Receives Game Status from Robo RIO over i2c and controls LEDs

include <bitswap.h>
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

#define NUMPIXELS 62// Number of LEDs in strip

// control the LEDs pins:
#define DATAPIN1    4
#define CLOCKPIN1   5
#define DATAPIN2    8
#define CLOCKPIN2   9
#define COLOR_ORDER GBR
#define CHIPSET APA102
#define BRIGHTNESS  250
#define FRAMES_PER_SECOND 60


const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
char gameMode = 'D';
char autonMode = 'S';
char gearPos = 'I';
char turret = 'U';
char win = 'N';
char alliance = 'R';
char climb = 'J'
unsigned char yoyoPos = 30;
unsigned char pegPos = 30;

int head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
int p2p_average = 0;
int sound_scale = 512;
int lightDelay = 0;

//Color Definitions
uint32_t auto_gearright = 0x0000FF; //Green
uint32_t auto_gearleft = 0xFFFF00; //Yellow
uint32_t auto_num5 = 0xFF00FF; //Pink
uint32_t auto_gearstraight = 0xFFFFFF; //white
uint32_t auto_num6 = 0x00FFFF; //Teal
uint32_t auto_num4 = 0xF000FF; //Purple

     // 'On' color (starts red)
uint32_t testColor = 0xFF0000;      // 'On' color (starts red)
uint32_t aimedColor = 0x00FF00;      // 'On' color (aimed = green)
uint32_t notaimedColor = 0xF000FF;      // 'On' color (not aimed = purple)
uint32_t alliancecolor = 0xFF0000;      // 'On' color (starts red)
uint32_t disablecolor = 0xFF0000;      // 'On' color (starts red)
uint32_t catyellowcolor = 0x444400;      // 'On' color (starts red)
uint32_t autonModecolor = 0x444400;      // 'On' color (starts red)
uint32_t color = 0xFF0000;
uint32_t teleColor = alliancecolor;


void setup()
{
  Wire.begin(84);                // join i2c bus with address #84
  TWBR = 12;
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output

  FastLED.addLeds<CHIPSET, LED_PIN,CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  FastLED.addLeds<CHIPSET, LED_PIN,CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );


  strip1.begin(); // Initialize pins for output
  strip1.show();  // Turn all LEDs off ASAP
  strip2.begin(); // Initialize pins for output
  strip2.show();  // Turn all LEDs off ASAP
  //strip.setBrightness(20);

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
      case 'X':
        testMode(); //Test Multi color
        break;
      default:
        disabledMode(); // Disabled Red
    }

  }

  //  strip.show();
}

void celebrationMode()
{
  for (int x = 0; x < NUMPIXELS; x++)
  {
    color = random();
    strip1.setPixelColor (x, color);
    strip2.setPixelColor (x, color);
  }
  strip1.show();
  strip2.show();
  delay(50);
}

void auton()
{
 
  for (int x = 0; x < NUMPIXELS; x++)
  {

    strip1.setPixelColor (x, alliancecolor);
    strip2.setPixelColor (x, alliancecolor);
  }
  while (gameMode == 'A')
  {
  
  for (int x = tail; x < head; x++)
  {
    strip1.setPixelColor (x, 0x00FF00);
    strip2.setPixelColor (x, 0x00FF00);
  }
    strip1.setPixelColor (tail-1, alliancecolor);
    strip2.setPixelColor (tail-1, alliancecolor);
  strip1.show();
  strip2.show();
  if (tail <= NUMPIXELS)
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
    
  for (int x = 0; x < NUMPIXELS; x++)
  {

    strip1.setPixelColor (x, teleColor);
    strip2.setPixelColor (x, teleColor);
  }
  strip1.show();
  strip2.show();
  delay(lightDelay);
 
  if (lightDelay >0)
  {
    for (int x = 0; x < NUMPIXELS; x++)
    {
  
      strip1.setPixelColor (x, 0);
      strip2.setPixelColor (x, 0);
    }
    strip1.show();
    strip2.show();
    delay(lightDelay);
  }
  for (int x = 0; x < NUMPIXELS; x++)
  {

    strip1.setPixelColor (x, teleColor);
    strip2.setPixelColor (x, teleColor);
  }
  strip1.show();
  strip2.show();

}
void testMode()
{
  for (int x = 0; x < 36; x++)
  {
    color = rand();
    strip1.setPixelColor (x, color);
    strip2.setPixelColor (x, color);
  }
  for (int x = 36; x < NUMPIXELS; x++)
  {
    color = 0x000000;
    strip1.setPixelColor (x, color);
    strip2.setPixelColor (x, color);
  }
  strip1.show();
  strip2.show();
  delay(100);
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
   

   int height = (peakToPeak * NUMPIXELS/2) / sound_scale ; // convert to volts
 
   for(i=NUMPIXELS/2; i>=0; i--) {
    if(i < NUMPIXELS/2 - height)
    {
      strip1.setPixelColor(i,0);
      strip2.setPixelColor(i,0);
      strip1.setPixelColor(NUMPIXELS-i,0);
      strip2.setPixelColor(NUMPIXELS-i,0);
    }
    else
    {
      strip1.setPixelColor(i,autonModecolor);
      strip2.setPixelColor(i,autonModecolor);
      strip1.setPixelColor(NUMPIXELS-i,autonModecolor);
      strip2.setPixelColor(NUMPIXELS-i,autonModecolor);
    }
  }
  strip1.show(); // Update strip
  strip2.show(); // Update strip
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
    turret = text[6];
  //   Serial.println(turret); 
    climb = text[7];
  //   Serial.println(climb);
      yoyoPos= text[8];
  //   Serial.println(yoyoPos);
      pegPos = text[9];
  //   Serial.println(pegPos);
   
  aimed = text[4];
  //    Serial.println(aimed);
  win = text[5];
  //   Serial.print(win);



  if (alliance == 'R')
  {
    alliancecolor = 0xFF0000;
  }
  else
  {
    alliancecolor = 0x0000FF;
  }
  if (aimed == 'X')
  {
    teleColor = aimedColor;
  }
  else if (aimed == 'Z')
  {
    teleColor = notaimedColor;
  }
  else
  {
    teleColor = alliancecolor;
  }
  if (ballPos == 'I')
  {
    lightDelay = 120;
  }
  else if (ballPos == 'S')
  {
    lightDelay = 40;
  }
   else
  {
   lightDelay = 0;
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
