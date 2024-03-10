#include <Arduino.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourceSPIFFS.h"
#include <ESP8266WiFi.h>
#include <I2S.h>

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
/*
  There's no practical limit on the amount of tape we can measure, but with the included
  audio files, the maximum length we can achieve is 11ft 11in.
*/
int maximumInches = 143;
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
// Rotation Calculation
//////
//////
/*
  There will be twice as many events per rotation as there are notches in the spool,
  because an event means both the transition LOW->HIGH and HIGH->LOW.
*/
int eventsPerFullRotation = notchesInSpool * 2; 
int minimumEventsToAnnounce = 3;
int eventCounter = 0;
unsigned long timeOfLastEvent = 0;
float eventTimeoutInMilliseconds = 750;
//////
//////


//////
//////
// Debug
//////
//////
#define STATE_LED_PIN D5
#define WARN_LED_PIN D7
bool ledState = false;
//////
//////


//////
//////
// IR Sensing
//////
//////
#define IR_SENSOR_PIN A0 
int irLowThreshold = 30;
int irHighThreshold = 100;
bool passedHighThreshold = false;
bool passedLowThreshold = false;
// To debounce IR pulses
float minimumTimeBetweenEvents = 2;
//////
//////


//////
//////
// Audio Filenames
//////
//////
String filePrefix = "/";
String fileSuffix = "_loud.mp3";
String youHaveUsedFile = filePrefix + "yhu" + fileSuffix;
String godBlessYouFile = filePrefix + "gby" + fileSuffix;
String inchFile = filePrefix + "inch" + fileSuffix;
String inchesFile = filePrefix + "inches" + fileSuffix;
String footFile = filePrefix + "foot" + fileSuffix;
String feetFile = filePrefix + "feet" + fileSuffix;
String usedTooMuch = filePrefix + "max" + fileSuffix;
//////
//////

//////
//////
// Function Declarations
//////
//////
String filenameForNumber(int number);
AudioFileSourceSPIFFS* fileForName(String name);
int eventsToInches(int eventCount);
bool eventsFinished();
void tapeMoved();
void resetCounters();
void startAnnouncement();
void queueBlessing(int feet, int inches);
void printBlessing(int feet, int inches);
void queueCurse();
void printCurse();
void eventLoop();
void sensorLoop();
void playQueuedAudio();
void playbackLoop();
void resetPlayback();
void addToQueue(String filename);
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

String audioFiles[6];
int queueArraySize = 0;
int queuePlaybackPosition = 0;
bool playbackInProgress = false;
//////
//////

void testAudio() {
  queueBlessing(1, 3);
  playQueuedAudio();
}

void setup()
{
  circumference = tapeRadiusInMillimetres * PI * 2;
  millimetresPerEvent = circumference/eventsPerFullRotation;

  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();
  pinMode(WARN_LED_PIN, OUTPUT);
  pinMode(STATE_LED_PIN, OUTPUT);

  out = new AudioOutputI2SNoDAC();
  out->SetOversampling(200);
  out->SetGain(2.0);
  Serial.println("Hello! How's the son?");

  delay(1000);
  testAudio();
}

void loop() {
  playbackLoop();

  // if audio is playing, skip the rest of the loop.
  if (playbackInProgress) { return; }
  sensorLoop();
  eventLoop();
}

void sensorLoop() {
  int value = analogRead(IR_SENSOR_PIN);

  if (value < irLowThreshold) {
    if (!passedLowThreshold) {
      tapeMoved();
    }
    passedLowThreshold = true;
    passedHighThreshold = false;
  } else if (value > irHighThreshold) {
    if (!passedHighThreshold) {
      tapeMoved();
    }
    passedLowThreshold = false;
    passedHighThreshold = true;
  }
}

/*
  Runs each loop
  Resets event counter if timeout has elapsed
  Triggers audio playback if required
*/ 
void eventLoop() {
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
    playbackInProgress = true;
    startAnnouncement();
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
  Triggered once per IR event
  Increments rotation event counter and time
*/
void tapeMoved() {
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

void startAnnouncement() {
  int inches = eventsToInches(eventCounter);
  int feet = inches / inchesPerFoot;
  int remainingInches = inches % inchesPerFoot;

  if (inches > maximumInches) {
    queueCurse();
    printCurse();
  } else {
    queueBlessing(feet, remainingInches);
    printBlessing(feet, remainingInches);
  }

  playQueuedAudio();
}

void resetCounters() {
  eventCounter = 0;
  timeOfLastEvent = 0;
}

void queueCurse() {
  addToQueue(usedTooMuch);
}

void printCurse() {
  Serial.println("You have wasted a load of sticky tape. Go to confession.");
}

/*
  Queue the relevant audio files for the measure
  tape length.
*/
void queueBlessing(int feet, int inches) {
  addToQueue(youHaveUsedFile);

  if (feet == 1) {
    addToQueue(filenameForNumber(feet));
    addToQueue(footFile);
  } else if (feet > 0) {
    addToQueue(filenameForNumber(feet));
    addToQueue(feetFile);
  }

  if (inches == 1) {
    addToQueue(filenameForNumber(inches));
    addToQueue(inchFile);
  } else if (inches > 0) {
    addToQueue(filenameForNumber(inches));
    addToQueue(inchesFile);
  }
  
  addToQueue(godBlessYouFile);
}

void addToQueue(String filename) {
  audioFiles[queueArraySize] = filename;
  queueArraySize++;
}

/*
  A mostly-debug function, to print the measurement to Serial, instead of
  playing audio.
*/
void printBlessing(int feet, int inches) {
  Serial.println("");
  Serial.println("----");

  Serial.print("You have used ");

  if (feet == 1) {
    Serial.print(feet);
    Serial.print(" foot ");
  } else if (feet > 0) {
    Serial.print(feet);
    Serial.print(" feet ");
  }

  if (inches == 1) {
    Serial.print(inches);
    Serial.print(" inch ");
  } else if (inches > 0) {
    Serial.print(inches);
    Serial.print(" inches ");
  }

  Serial.println("of sticky tape. God bless you.");
  Serial.print("@");
  Serial.println(millis());
  Serial.println("----");
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

uint32_t i2sACC;
uint16_t err;

void writeDAC(uint16_t DAC) {
  for (uint8_t i=0;i<32;i++) {
    i2sACC=i2sACC<<1;

    if(DAC >= err) {
      i2sACC|=1;
      err += 0xFFFF-DAC;
    } else {
      err -= DAC;
    }
  }

  i2s_write_sample(i2sACC);
}

void startPlayback() {
  for (int j=1000;j>1;j--) {
    writeDAC(0x8000-32767*j/1000);
  }
  mp3->begin(fileForName(audioFiles[queuePlaybackPosition]), out);
}

void stopPlayback() {
  for (int j=0;j<1000;j++) {
    writeDAC(0x8000-32767*j/1000);
  }
  mp3->stop();
}

void playbackLoop() {
  if (!mp3) { return; }
  if (!mp3->isRunning()) { return; }
  if (mp3->loop()) { return; }

  /* If the file is finished playing, try moving onto the next file until there are none left. */
  if (queuePlaybackPosition < queueArraySize - 1) {
    queuePlaybackPosition++;
    // mp3->stop();
    stopPlayback();
    // mp3->begin(fileForName(audioFiles[queuePlaybackPosition]), out);
    startPlayback();
  } else {
    // mp3->stop();
    stopPlayback();
    resetPlayback();
  }
}

void playQueuedAudio() {
  mp3 = new AudioGeneratorMP3();
  // Start playing the first audio file in the queue.
  mp3->begin(fileForName(audioFiles[queuePlaybackPosition]), out);
}

void resetPlayback() {
  queueArraySize = 0;
  queuePlaybackPosition = 0;
  mp3 = NULL;
  playbackInProgress = false;
}