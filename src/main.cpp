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
int eventsPerFullRotation = notchesInSpool * 2; 
int minimumEventsToAnnounce = 3;
int eventCounter = 0;
unsigned long timeOfLastEvent = 0;
float eventTimeoutInMilliseconds = 250;
//////
//////

//////
//////
// Audio Filenames
//////
//////
String filePrefix = "/";
String fileSuffix = ".mp3";
String youHaveUsed = filePrefix + "yhy" + fileSuffix;
String godBlessYou = filePrefix + "gby" + fileSuffix;
String inch = filePrefix + "inch" + fileSuffix;
String inches = filePrefix + "inches" + fileSuffix;
String foot = filePrefix + "foot" + fileSuffix;
String feet = filePrefix + "feet" + fileSuffix;
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

  Serial.begin(115200);
}

void loop() {

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

/*
  Callback triggered upon interrupt from IR sensor
  Increments rotation event counter and time
*/
IRAM_ATTR void tapeMoved() {
  eventCounter += 1;
  timeOfLastEvent = millis();
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