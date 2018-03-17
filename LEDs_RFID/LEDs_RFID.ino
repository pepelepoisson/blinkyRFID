#include <Arduino.h>
// Information about hardware connections, functions and definitions
#include "FastLED.h"
FASTLED_USING_NAMESPACE
/**
   ----------------------------------------------------------------------------
   Use MFRC522 library. Ref: https://github.com/miguelbalboa/rfid

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno           Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <MFRC522.h>


//  Declare pins
#define DATA_PIN 2  // Strip data pin
#define ECHO_TO_SERIAL   1 // echo data to serial port
#define NUM_LEDS 330  // Number of leds in strip
#define BRIGHTNESS 150  // Set LEDS brightness
#define FRAMES_PER_SECOND  120
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
#define SER_NUM_LEN 4

CRGB leds[NUM_LEDS];  // Define the array of leds
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

static uint8_t default_hue=50;
static uint8_t hue = default_hue;
/*
 * 50=yellow
 * 100=green
 * 160=blue
 * 0=red
 */
static uint8_t hue_step = 24;
static uint8_t tail = 180;
static int step = -1;
static uint8_t turned_on_leds = 0;
unsigned char serNum[SER_NUM_LEN];
bool inField = false;
uint8_t gFrameCount = 0; // Inc by 1 for each Frame of Trasition, New/Changed connection(s) pattern

CRGB clr1;
CRGB clr2;
uint8_t speed;
uint8_t loc1;
uint8_t loc2;
uint8_t ran1;
uint8_t ran2;

uint8_t fadeval = 224; 

CHSV hsvval(hue,255,BRIGHTNESS);

CRGBPalette16 currentPalette = PartyColors_p;
TBlendType currentBlending = LINEARBLEND; 

unsigned char currentCard[SER_NUM_LEN]={245,110,84,68};
unsigned char card1[SER_NUM_LEN]={4,218,28,66};
unsigned char card2[SER_NUM_LEN]={231,97,207,83};
unsigned char card3[SER_NUM_LEN]={5,175,248,67};
unsigned char card4[SER_NUM_LEN]={4,189,233,114};
unsigned char card5[SER_NUM_LEN]={21,240,18,83};
unsigned char card6[SER_NUM_LEN]={217,154,57,174};
unsigned char card7[SER_NUM_LEN]={182,228,202,94};
unsigned char card8[SER_NUM_LEN]={245,110,84,68};
unsigned char card9[SER_NUM_LEN]={102,191,33,43};

bool matchSerNum() {
  for (int i = 0; i < SER_NUM_LEN; ++i) {
    if (serNum[i] != mfrc522.uid.uidByte[i]) {
      return false;
    }
  }
  return true;
}

void copySerNum() {
  for (int i = 0; i < SER_NUM_LEN; ++i) {
    serNum[i] = mfrc522.uid.uidByte[i];
    currentCard[i]=serNum[i];
  }
}

void clearSerNum() {
  for (int i = 0; i < SER_NUM_LEN; ++i) {
    serNum[i] = 0;
  }
}

void printSerNum() {
  Serial.print("serialNumber:");

  for (int i = 0; i < SER_NUM_LEN; ++i) {
    if (i != 0) {
      Serial.print("-");
    }
    //Serial.print(serNum[i], HEX);
    Serial.print(serNum[i]);
  }
  Serial.println(" ");
}




uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void fadeall() {
  for (int i = turned_on_leds+1; i < NUM_LEDS; i++) {
    leds[i].nscale8(tail);
  }
}
void alloff() {
  for (int i = NUM_LEDS-1; i >=0; i--) {
    leds[i]=CRGB::Black;
  }
}


void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void redGlitter() {
  gFrameCount += 1;
  if (gFrameCount % 4 == 1) { // Slow down frame rate
    for ( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(HUE_RED, 0, random8() < 60 ? random8() : random8(64));
    }
  }
}

void blendwave() {

  speed = beatsin8(6,0,255);

  clr1 = blend(CHSV(beatsin8(3,0,255),255,255), CHSV(beatsin8(4,0,255),255,255), speed);
  clr2 = blend(CHSV(beatsin8(4,0,255),255,255), CHSV(beatsin8(3,0,255),255,255), speed);

  loc1 = beatsin8(10,0,NUM_LEDS-1);
  
  fill_gradient_RGB(leds, 0, clr2, loc1, clr1);
  fill_gradient_RGB(leds, loc1, clr2, NUM_LEDS-1, clr1);

} // blendwave()

void dot_beat() {

  uint8_t inner = beatsin8(bpm, NUM_LEDS/4, NUM_LEDS/4*3);    // Move 1/4 to 3/4
  uint8_t outer = beatsin8(bpm, 0, NUM_LEDS-1);               // Move entire length
  uint8_t middle = beatsin8(bpm, NUM_LEDS/3, NUM_LEDS/3*2);   // Move 1/3 to 2/3

  leds[middle] = CRGB::Purple;
  leds[inner] = CRGB::Blue;
  leds[outer] = CRGB::Aqua;

  nscale8(leds,NUM_LEDS,fadeval);                             // Fade the entire array. Or for just a few LED's, use  nscale8(&leds[2], 5, fadeval);

} // dot_beat()

void sawtooth() {

  int bpm = 60;
  int ms_per_beat = 60000/bpm;                                // 500ms per beat, where 60,000 = 60 seconds * 1000 ms 
  int ms_per_led = 60000/bpm/NUM_LEDS;

  int cur_led = ((millis() % ms_per_beat) / ms_per_led)%(NUM_LEDS);     // Using millis to count up the strand, with %NUM_LEDS at the end as a safety factor.

  if (cur_led == 0)
   fill_solid(leds, NUM_LEDS, CRGB::Black);
  else
    leds[cur_led] = ColorFromPalette(currentPalette, 0, 255, currentBlending);    // I prefer to use palettes instead of CHSV or CRGB assignments.

} // sawtooth()


void CheckCard() {
  bool match;
  //Serial.println("Check card");
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      
      inField = true;
      match = matchSerNum();
   
      if (!match) {
        copySerNum();
        printSerNum();
      }

      mfrc522.PICC_HaltA();
      return;
    }
  }

  // FIXME: I can not send the out of field info with this lib.
  //        It trigger right away for now.
  if (inField) {
    inField = false;
    // printSerNum();
    clearSerNum();
  }

  mfrc522.PICC_HaltA();
}

void setup(void)
{
  
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  set_max_power_in_volts_and_milliamps(5, 1500); 
  
  //delay(500);
  //alloff();
  
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  
  Serial.println("Demarrage");
}

void loop(void){

  //CheckCard();
    if (card1[0]==currentCard[0] && card1[1]==currentCard[1]&& card1[2]==currentCard[2]&& card1[3]==currentCard[3]){bpm();}
    else if (card2[0]==currentCard[0] && card2[1]==currentCard[1]&& card2[2]==currentCard[2]&& card2[3]==currentCard[3]){blendwave();}
    else if (card3[0]==currentCard[0] && card3[1]==currentCard[1]&& card3[2]==currentCard[2]&& card3[3]==currentCard[3]){redGlitter();}
    else if (card4[0]==currentCard[0] && card4[1]==currentCard[1]&& card4[2]==currentCard[2]&& card4[3]==currentCard[3]){confetti();}
    else if (card5[0]==currentCard[0] && card5[1]==currentCard[1]&& card5[2]==currentCard[2]&& card5[3]==currentCard[3]){alloff();}
    else if (card6[0]==currentCard[0] && card6[1]==currentCard[1]&& card6[2]==currentCard[2]&& card6[3]==currentCard[3]){sinelon();}
    else if (card7[0]==currentCard[0] && card7[1]==currentCard[1]&& card7[2]==currentCard[2]&& card7[3]==currentCard[3]){sawtooth();}
    //else if (card8[0]==currentCard[0] && card8[1]==currentCard[1]&& card8[2]==currentCard[2]&& card8[3]==currentCard[3]){dot_beat();}
    else if (card8[0]==currentCard[0] && card8[1]==currentCard[1]&& card8[2]==currentCard[2]&& card8[3]==currentCard[3]){rainbow();}
    else if (card9[0]==currentCard[0] && card9[1]==currentCard[1]&& card9[2]==currentCard[2]&& card9[3]==currentCard[3]){juggle();}
    //else {rainbow();}
    //else {hue=currentCard[0];CHSV hsvval(hue,255,BRIGHTNESS);fill_solid(leds,NUM_LEDS, hsvval);}
    else {fill_gradient(leds, NUM_LEDS, CHSV(currentCard[0], 255,255), CHSV(currentCard[2],255,255), SHORTEST_HUES);}
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS( 200 ) { 
    CheckCard();  
    Serial.print("currentCard: ");
    for (int i = 0; i < SER_NUM_LEN; ++i) {
      if (i != 0) {
        Serial.print("-");
      }
      Serial.print(currentCard[i]);
    } 
    Serial.println(" ");
    } // change patterns periodically

}


