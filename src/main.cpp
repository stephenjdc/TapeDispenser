#include <Arduino.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourceSPIFFS.h"
#include <ESP8266WiFi.h>

/*
  NOTES

  'events' are when the signal from the IR sensor changes
  i.e. both HIGH->LOW and LOW->HIGH transitions

*/

//////
//////
// Tape Config
//////
//////
int inchesPerFoot = 12;
float millimetresPerInch = 25.4;
/*
  Ideally, the distance from the centerpoint of the spool
  to 1/2 way along the width of the tape. This splits the difference in errors;
  undershoot slightly when full, and overshoot slightly when near-empty. This will
  be well within 1-inch resolution.
*/
float tapeRadiusInMillimetres = 22.5;
/*
  The number of openings in the spool, i.e. the number of times the 
  IR beam will get broken per 1 revolution.
*/
int notchesInSpool = 16;
/*
  For ease of configuration, we'll set these values during setup.
  For efficiency's sake, since they won't change during operation,
  we won't bother calculating them every time.
*/
float circumference;
float millimetresPerEvent;
//////
//////


//////
//////
// Rotation Tracking
//////
//////
int eventsPerFullRotation = notchesInSpool; 
int minimumEventsToAnnounce = 3;
int eventCounter = 0;
unsigned long timeOfLastEvent = 0;
float eventTimeoutInMilliseconds = 750;
// To debounce IR pulses
float minimumTimeBetweenEvents = 8;
#define IR_SENSOR_PIN D6 
unsigned long timeOfLastAnnouncement = 0;
float minimumTimeBetweenAnouncements = 5000;
#define STATE_LED_PIN D5
#define WARN_LED_PIN D7
bool ledState = false;
int lastIRState = 0;
//////
//////

//////
//////
// Audio Filenames
//////
//////
String filePrefix = "/";
String fileSuffix = ".mp3";
String youHaveUsedFile = filePrefix + "yhy" + fileSuffix;
String godBlessYouFile = filePrefix + "gby" + fileSuffix;
String inchFile = filePrefix + "inch" + fileSuffix;
String inchesFile = filePrefix + "inches" + fileSuffix;
String footFile = filePrefix + "foot" + fileSuffix;
String feetFile = filePrefix + "feet" + fileSuffix;
//////
//////

//////
//////
// Function Declarations
//////
//////
String filenameForNumber(int number);
AudioFileSourceSPIFFS* fileForName(String name);
void triggerBlessing();
void tapeMoved();
bool eventsFinished();
void resetCounters();
void triggerBlessing();
int eventsToInches(int eventCount);
void playBlessingForInches(int inches);
void checkEvents();
//////
//////

//////
//////
// Audio playback
//////
//////
AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
//////
//////

void setup()
{
  circumference = tapeRadiusInMillimetres * PI * 2;
  millimetresPerEvent = circumference/eventsPerFullRotation;

  WiFi.mode(WIFI_OFF);
  // SPIFFS.begin();
  pinMode(WARN_LED_PIN, OUTPUT);
  pinMode(STATE_LED_PIN, OUTPUT);

  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), tapeMoved, FALLING);

  Serial.begin(115200);
  Serial.println("Hello! How's the son?");
}

void loop() {
  checkEvents();
}

/*
  Runs each loop
  Resets event counter if timeout has elapsed
  Triggers audio playback if required
*/ 
void checkEvents() {
  if (timeOfLastEvent == 0) {
    return;
  }

  if (millis() - eventTimeoutInMilliseconds < timeOfLastEvent) {
    return;
  }

  /*
    If the counter is above the minimum threshold, play announcement
    and reset. If it's below the threshold, just reset.
  */
  if (eventCounter > minimumEventsToAnnounce) {
    triggerBlessing();
    resetCounters();
  } else {
    resetCounters();
  }
}

void showDebugLEDs(unsigned long time, bool debounced) {
  digitalWrite(WARN_LED_PIN, debounced);

  if (ledState) {
    digitalWrite(STATE_LED_PIN, LOW);
    ledState = false;
  } else {
    digitalWrite(STATE_LED_PIN, HIGH);
    ledState = true;
  }
}
/*
  Callback triggered upon interrupt from IR sensor
  Increments rotation event counter and time
*/
IRAM_ATTR void tapeMoved() {
  // Skip if sensor reads HIGH
  if (digitalRead(IR_SENSOR_PIN)) {
      return;
  }
  unsigned long time = millis();
  // Debounce signal from IR led
  if ((time - timeOfLastEvent) < minimumTimeBetweenEvents) {
    showDebugLEDs(time, true);
    return;
  } 
  showDebugLEDs(time, false);

  // Set event properties
  eventCounter += 1;  
  timeOfLastEvent = time;
}

/*
  Convert the specified eventCount to the nearest integer inch
*/
int eventsToInches(int eventCount) {
  float inches = (millimetresPerEvent * eventCount) / millimetresPerInch;
  // Cheap and cheerfully round to the nearest int
  inches += 0.5;
  return (int)inches;
}

void triggerBlessing() {
  playBlessingForInches(eventsToInches(eventCounter));
}

void resetCounters() {
  eventCounter = 0;
  timeOfLastEvent = 0;
}

void playBlessingForInches(int inches) {
  Serial.println("");
  Serial.println("");
  Serial.println("");

  int feet = inches / inchesPerFoot;
  int remainingInches = inches % inchesPerFoot;

  Serial.print("You have used ");
  // Serial.println(youHaveUsedFile);

  if (feet > 0) {
    Serial.print(feet);
    // Serial.println(filenameForNumber(feet));
    if (feet == 1) {
      Serial.print(" foot ");
      // Serial.println(footFile);
    } else {
      Serial.print(" feet ");
      // Serial.println(feetFile);
    }
  }

  if (remainingInches > 0) {
    Serial.print(remainingInches);
    // Serial.println(filenameForNumber(remainingInches));
    if (remainingInches == 1) {
      // Serial.println(inchFile);
      Serial.print(" inch ");
    } else {
      Serial.print(" inches ");
      // Serial.println(inchesFile);
    }
  }

  // Serial.println(godBlessYouFile);
  Serial.println("of sticky tape. God bless you.");
  Serial.print("@");
  Serial.println(millis());
  Serial.println("");
  Serial.println("");
  Serial.println("");
}

//////
//////
// Audio playback
//////
//////

// Generate the filename for number audioclips
String filenameForNumber(int number) {
    return filePrefix + String(number) + fileSuffix;
}

AudioFileSourceSPIFFS* fileForName(String name) {
  return new AudioFileSourceSPIFFS(name.c_str());
}