#include <FastLED.h>
#include <IRremote.h>
#include <EEPROM.h>

// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
//#define irdebug;
//#define debug;
#define DATA_PIN 11
#define CLOCK_PIN 13
#define LED_CHIPSET APA102
#define COLOR_ORDER BGR
#define ledCount 150
#define IR_PIN 10
#define irbtncount 35
#define addressR 0
#define addressG 1
#define addressB 2
#define addresslastprogram 3
//#define

// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
IRrecv irrecv(IR_PIN);
decode_results results;
CRGB actualcolors[ledCount];                                              // Create a buffer for holding the colors (3 bytes per color).  --- ATTENTION! used from old rainbow led program
uint8_t chosencolor[3];                                                   // color variable for actual user chosen color
CRGB chosencolorledarray[ledCount];                                       // color array for actual user chosen color
CRGB eepromcolor;                                                         // color variable filled with last saved user chosen color
CRGB eepromcolorarray[ledCount];                                          // color array filled with last saved user chosen color
CRGB blackarray[ledCount];
uint32_t chosenprogram = ;                                             // program number
uint32_t lastchosenprogram;                                               // last program number
uint8_t brightness = 31;                                                  // Set the brightness to use (the maximum is 31).
boolean globalshutdown = false;                                           // global boolean for standby aka "poweroff" - used on bootup
boolean whiteoverlay = false;                                             // global boolean for "program4" aka white overlay in steady colors
uint32_t actualircode;
bool bootup = true;
const bool debug = false;
const bool irdebug = false;
uint8_t actualindexofdimarray;
uint8_t gHue = 0;                                                         // rotating "base color" used by many of the patterns
uint8_t FRAMES_PER_SECOND = 120;

// two-dimensional array - matching of ir code to program
uint32_t irtofunction[2][irbtncount] = {
  {
    // colors
    0xFF1AE5,
    0xFF9A65,
    0xFFA25D,
    0xFF22DD,
    0xFF2AD5,
    0xFFAA55,
    0xFF926D,
    0xFF12ED,
    0xFF0AF5,
    0xFF8A75,
    0xFFB24D,
    0xFF32CD,
    0xFF38C7,
    0xFFB847,
    0xFF7887,
    0xFFF807,
    0xFF18E7,
    0xFF9867,
    0xFF58A7,
    0xFFD827,
    // and now controls
    0xFF28D7,
    0xFF08F7,
    0xFFA857,
    0xFF8877,
    0xFF6897,
    0xFF48B7,
    0xFFE817,
    0xFFC837,
    0xFF20DF,
    0xFFA05F,
    0xFFE01F,
    0xFF30CF,
    0xFF02FD,
    0xFF3AC5,
    0xFFBA45
  },
  {
    // usercolor hard 20x  1-20
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    // usercolor soft 6x  21-26
    5, 5, 5, 5, 5, 5,
    // speed programs 2x  27+28
    6, 6,
    // real programs  3x  29-35
    1, 2, 4, 7, 0, 8, 9
  }
};


const CRGB color[20] = {
  CRGB(255, 0, 0),
  CRGB(0, 255, 0),
  CRGB(0, 0, 255),
  CRGB(255, 255, 255),
  CRGB(200, 75, 15),
  CRGB(75, 200, 5),
  CRGB(15, 15, 255),
  CRGB(255, 90, 90),
  CRGB(150, 40, 5),
  CRGB(45, 45, 255),
  CRGB(0, 0, 120),
  CRGB(255, 80, 80),
  CRGB(200, 75, 7),
  CRGB(8, 8, 200),
  CRGB(255, 0, 220),
  CRGB(85, 85, 255),
  CRGB(255, 240, 0),
  CRGB(5, 5, 255),
  CRGB(200, 25, 180),
  CRGB(50, 50, 220)
};


// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// EEPROM shit functions (getter + setter)

CRGB eepromgetrgb () {
  uint8_t r = EEPROM.read(addressR);
  uint8_t g = EEPROM.read(addressG);
  uint8_t b = EEPROM.read(addressB);
  return CRGB(r, g, b);
}

void eepromsetrgb (uint8_t r, uint8_t g, uint8_t b) {
  Serial.println("eepromsetrgb");
  Serial.print("R: ");
  Serial.println(r);
  Serial.print("G: ");
  Serial.println(g);
  Serial.print("B: ");
  Serial.println(b);

  EEPROM.write(addressR, r);
  EEPROM.write(addressG, g);
  EEPROM.write(addressB, b);

  Serial.print("R: ");
  Serial.println(EEPROM.read(addressR));
  Serial.print("G: ");
  Serial.println(EEPROM.read(addressG));
  Serial.print("B: ");
  Serial.println(EEPROM.read(addressB));
}

void eepromsaveprogram (uint8_t chosenprogram) {
  EEPROM.write(addresslastprogram, chosenprogram);
  Serial.print("Program saved: ");
  Serial.println(chosenprogram);
}

uint8_t eepromgetprogram () {
  Serial.print("Program saver read: ");
  Serial.println(EEPROM.read(addresslastprogram));
  return EEPROM.read(addresslastprogram);
}

// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// Fillarray void used to setup solid color arrays
#define FILLARRAY(a,n) a[0]=n, memcpy( ((char*)a)+sizeof(a[0]), a, sizeof(a)-sizeof(a[0]) );

// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################

// functions for the ir reciever
void toggleboardled(int delayms) {
  digitalWrite(13, HIGH);
  delay(delayms);
  digitalWrite(13, LOW);
  delay(delayms);
}

uint8_t getindexofdimarray(uint32_t actualircode) {
  uint8_t arrayindex = 255;
  for (int index = 0; index <= irbtncount - 1; index++) {

    //Serial.print("actualircode: ");
    //Serial.println(actualircode);
    //Serial.print("tested ir value: ");
    //Serial.println(irtofunction[0][index], HEX);
    if (actualircode == irtofunction[0][index]) {
      arrayindex = index;


    }

    //delay(100);
  }
  return arrayindex;
}


void protocolldetection(decode_results results) {
  if (irrecv.decode(&results)) {
    if (results.decode_type == NEC) {
      Serial.print("NEC: ");
      Serial.println(results.value, HEX);
    } else if (results.decode_type == SONY) {
      Serial.print("SONY: ");
      Serial.println(results.value, HEX);
    } else if (results.decode_type == RC5) {
      Serial.print("RC5: ");
      Serial.println(results.value, HEX);
    } else if (results.decode_type == RC6) {
      Serial.print("RC6: ");
      Serial.println(results.value, HEX);
    } else if (results.decode_type == UNKNOWN) {
      Serial.print("UNKNOWN: ");
      Serial.println(results.value, HEX);
    }

    if (!(results.value == 0xFFFFFFFF)) {
      if (results.decode_type == NEC) {

        actualircode = results.value;
      }
    }
    irrecv.resume(); // Receive the next value

  }

}


void mainirloop() {

  protocolldetection(results);

  // get index for ir code
  actualindexofdimarray = getindexofdimarray(actualircode);

  if (actualindexofdimarray <= irbtncount) {
    // get program number for index
    if (irtofunction[1][actualindexofdimarray] < 8) {
      /*lastchosenprogram = */ chosenprogram = irtofunction[1][actualindexofdimarray];
    } else {
      chosenprogram = irtofunction[1][actualindexofdimarray];
    }
  }

  if (irdebug) {
    delay(20);
    Serial.print("chosenprogram: ");
    Serial.print(chosenprogram);

    Serial.print("     lastchosenprogram: ");
    Serial.print(lastchosenprogram);

    Serial.print("     actualindexofdimarray: ");
    Serial.print(actualindexofdimarray);

    Serial.print("     actualircode: ");
    Serial.println(actualircode, HEX);

  }
}

// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
/* Converts a color from HSV to RGB.
   h is hue, as a number between 0 and 360.
   s is the saturation, as a number between 0 and 255.
   v is the value, as a number between 0 and 255. */
CRGB hsvToRgb(uint16_t h, uint8_t s, uint8_t v) {
  uint8_t f = (h % 60) * 255 / 60;
  uint8_t p = (255 - s) * (uint16_t)v / 255;
  uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
  uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
  uint8_t r = 0, g = 0, b = 0;
  switch ((h / 60) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }
  return CRGB(r, g, b);
}

void mainledloop() {
  // just a wrapper for now
  sendledarray();
}

void sendledarray() {
  /*
    Serial.print(brightness);
    Serial.print(" ");
    Serial.print(chosenprogram);
    Serial.print(" ");
    Serial.println(lastchosenprogram);
  */

  //ledStrip.write(chosencolorledarray, ledCount, brightness);
  FastLED.setBrightness(map(brightness, 0, 31, 0, 255) );
  FastLED.show();
}

// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################

// for all "program*" functions defined in programdecision

void programdecision(uint32_t actualircode) {

  // run program with number
  switch (chosenprogram) {
    case 0:
      if (bootup != true) {
        Serial.println("LED Shutdown");
        eepromsaveprogram(chosenprogram);
        programglobalshutdown();
        if (!globalshutdown) {
          Serial.println("Turn off shutdown");
          chosenprogram = lastchosenprogram;
        } else if (globalshutdown) {
          Serial.println("Turn on shutdown");
          chosenprogram = 10;
        }
      } else {
        chosenprogram = 10;
      }
      break;
    case 1:
      //Serial.println("Rainbow");
      FRAMES_PER_SECOND = 200;
      if (chosenprogram != lastchosenprogram) {
        if (bootup != true) {
          eepromsaveprogram(chosenprogram);
        }
      }

      programrainbow();
      break;
    case 2:
      //Serial.println("infading");
      break;
    case 3:
      //Serial.println("usercolor hard");
      programhardusercolor();
      if (bootup != true) {
        eepromsaveprogram(chosenprogram);
      }
      chosenprogram = 10;
      break;
    case 4:
      //Serial.println("white overlay");
      FRAMES_PER_SECOND = 60;
      programwhiteoverlay();
      break;
    case 5:
      //Serial.println("usercolor soft");
      //programsoftusercolor(actualircode, );
      break;
    case 6:
      //Serial.println("bpm");
      FRAMES_PER_SECOND = 250;
      programbpm();
      break;
    case 7:
      FRAMES_PER_SECOND = 60;
      programconfetti();
      break;
    case 8:
      //Serial.println("Brightness Up");
      programbrightnessup();
      chosenprogram = lastchosenprogram;
      break;
    case 9:
      //Serial.println("Brightness down");
      programbrightnessdown();
      chosenprogram = lastchosenprogram;
      break;
    case 10:
      // do nothing
      programnothing();
      break;
  }
}

void programconfetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( chosencolorledarray, ledCount, 1);
  int pos = random16(ledCount);
  chosencolorledarray[pos] += CHSV( gHue, 180, 255);
}

void programbpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = RainbowColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 30, 255);
  for( int i = 0; i < ledCount; i++) { //9948
    chosencolorledarray[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void fill_rainbow_chris( struct CRGB * pFirstLED, int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t val )
{
  CHSV hsv;
  hsv.hue = initialhue;
  hsv.val = val;
  hsv.sat = 240;
  for ( int i = 0; i < numToFill; i++) {
    pFirstLED[i] = hsv;
    hsv.hue += deltahue;
  }
}

void programnothing() {
  // do nothing
  if (debug) {
    Serial.println("nothing");
  }
}

void programrainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow_chris( chosencolorledarray, ledCount, gHue, 7, 255);
}

void programwhiteoverlay() {
  //programrainbow();
  fill_rainbow_chris( chosencolorledarray, ledCount, gHue, 12, 110);
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    chosencolorledarray[ random16(ledCount) ] += CRGB::White;
  }
}

void programglobalshutdown() {
  if (!globalshutdown) {
    Serial.println("shutdown in progress...");
    for (int i = 0; i <= ledCount; i++) {
      chosencolorledarray[i] = CRGB(0, 0, 0);
    }
    //ledStrip.write(blackarray, ledCount, brightness);
    globalshutdown = true;
  } else if (globalshutdown) {
    globalshutdown = false;
  }
}


void programhardusercolor() {
  CRGB hardcolor = CRGB(0, 0, 0);
  if (bootup) {
    hardcolor = eepromgetrgb();
    Serial.println("color from eeprom");
  } else {
    hardcolor = color[actualindexofdimarray];
    Serial.println("color from ir code");
  }
  // set global chosen color
  chosencolor[0] = hardcolor.red;
  chosencolor[1] = hardcolor.green;
  chosencolor[2] = hardcolor.blue;
  Serial.println("hard user color:");
  Serial.print("color array: ");
  Serial.print(color[actualindexofdimarray].red);
  Serial.print(" ");
  Serial.print(color[actualindexofdimarray].green);
  Serial.print(" ");
  Serial.println(color[actualindexofdimarray].blue);
  Serial.print("local hardcolor array: ");
  Serial.print(hardcolor.red);
  Serial.print(" ");
  Serial.print(hardcolor.green);
  Serial.print(" ");
  Serial.println(hardcolor.blue);

  for (int i = 0; i <= ledCount; i++) {
    chosencolorledarray[i] = hardcolor;
  }

  if (bootup != true) {
    Serial.println("user hard color save");
    eepromsetrgb(hardcolor.red, hardcolor.green, hardcolor.blue);
  }

}

void programsoftusercolor(uint32_t actualircode, uint8_t chosencolor[]) {

  switch (actualircode) {
    case 0xFF28D7:
      // red up
      break;
    case 0xFF08F7:
      // green up
      break;
    case 0xFFA857:
      // blue up
      break;
    case 0xFF8877:
      // red down
      break;
    case 0xFF6897:
      // green down
      break;
    case 0xFF48B7:
      // blue down
      break;
  }
}

void programbrightnessup() {
  if (brightness < 31) {
    if (brightness > 1) {
      if (brightness == 27) {
        brightness = brightness + 3;
      } else {
        brightness = brightness + 4;
      }
    }
    brightness++;
  }
  Serial.print("brightness up: ");
  Serial.println(brightness, DEC);
}

void programbrightnessdown() {
  if (brightness > 1) {
    if (brightness > 5) {
      brightness = brightness - 4;
    } else {
      brightness--;
    }
  }
  Serial.print("brightness down: ");
  Serial.println(brightness, DEC);
}

void propgramsavecolor(int r, int g, int b) {
  Serial.println("Function programsavecolor()");

}




// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //while (!Serial.available() ) {
  //}
  Serial.println("bootup delay");
  if (bootup) {
    if (chosenprogram == 10) {
      chosenprogram = eepromgetprogram();
      Serial.print("chosenprogram from save: ");
      Serial.println(chosenprogram);
    }
  }
  FastLED.addLeds<LED_CHIPSET, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(chosencolorledarray, ledCount);
}

void loop() {
  mainirloop();
  programdecision(actualircode);
  mainledloop();
  lastchosenprogram = chosenprogram;
  actualircode = 0;
  globalshutdown = false;
  if (bootup) {
    bootup = false;
  }
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;
  }
}


// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################
// ###############################################################










