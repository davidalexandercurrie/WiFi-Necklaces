#include <Arduino.h>

#include <painlessMesh.h>

#include "SSD1306Wire.h"
SSD1306Wire display(0x3c, 21, 22);

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define LED 2 // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

#define MESH_SSID "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage();                                                // Prototype
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

#include "SoundData.h";
#include "XT_DAC_Audio.h";

#define NORMAL_SPEED 64 // These are the playback speeds, change to
#define FAST_SPEED 16   // see the effect on the sound sample. 1 is default speed
#define SLOW_SPEED 1    // >1 faster, <1 slower, 2 would be twice as fast, 0.5 half as fast

// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 27 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 24 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

XT_DAC_Audio_Class DacAudio(25, 0); // Create the main player class object.
                                    // Use GPIO 25, one of the 2 DAC pins and timer 0

// create an object of type XT_Wav_Class that is used by
// the dac audio class (above), passing wav data as parameter.
XT_Wav_Class Samples(SampleOne);
XT_Wav_Class Samples2(SampleTwo);
XT_Wav_Class Samples3(SampleThree);
XT_Wav_Class Samples4(SampleFour);

int WaitTime[5] = {25, 50, 100, 200, 400};

// SpeedArray contains the order in which the code will playback the sample at the designated speeds
float SpeedArray[] = {NORMAL_SPEED, FAST_SPEED, SLOW_SPEED};
uint8_t NoOfSpeeds = 3; // Number of difference speeds in the Speed array above
uint8_t SpeedIdx = 0;   // In effect when the checks in the main loop are made this will increment to 0
uint8_t turns = 0;
int period = 1000;
int counter = 0;
unsigned long time_now = 0;
int oldConnections = 0;
int randomNotes = 0;

void setup()
{
  delay(1); // Allow system to settle, otherwise garbage can play for first second

  display.init();

  // display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration())
    {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  display.drawString(20, 20, "Searching...");
  display.flipScreenVertically();
}

void loop()
{
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
  digitalWrite(LED, !onFlag);

  if (mesh.getNodeList().size() != oldConnections)
  {
    randomNotes = random(0, 5);
    display.clear();
    if (mesh.getNodeList().size() > 0)
    {
      display.drawString(20, 20, String(mesh.getNodeList().size() + 1) + " Connected");
    }
    else
    {
      display.drawString(20, 20, "Searching...");
    }
    oldConnections = mesh.getNodeList().size();
  }

  display.display();

  DacAudio.FillBuffer(); // Fill the sound buffer with data, required once in your main loop
                         // Has it completed?

  if (millis() > time_now + period)
  {
    time_now = millis();
    // Not Playing, check if played last speed, if so reset SpeedIdx back to 0

    if (turns % 4 == 0)
    {
      DacAudio.Play(&Samples);
      if (SpeedIdx == NoOfSpeeds)
        SpeedIdx = 0;
      Samples.Speed = SpeedArray[SpeedIdx];
      pixels.setPixelColor((counter % 24), pixels.Color(random(0, 255), random(0, 255), 0));
    }
    else if (turns % 4 == 1)
    {
      DacAudio.Play(&Samples2);
      if (SpeedIdx == NoOfSpeeds)
        SpeedIdx = 0;
      Samples2.Speed = SpeedArray[SpeedIdx];
      pixels.setPixelColor((counter % 24), pixels.Color(random(0, 255), 0, random(0, 255)));
    }
    else if (turns % 4 == 2)
    {
      DacAudio.Play(&Samples3);
      if (SpeedIdx == NoOfSpeeds)
        SpeedIdx = 0;
      Samples3.Speed = SpeedArray[SpeedIdx];
      pixels.setPixelColor((counter % 24), pixels.Color(0, random(0, 255), random(0, 255)));
    }
    else if (turns % 4 == 3)
    {
      DacAudio.Play(&Samples4);
      if (SpeedIdx == NoOfSpeeds)
        SpeedIdx = 0;
      Samples4.Speed = SpeedArray[SpeedIdx];
      pixels.setPixelColor((counter % 24), pixels.Color(random(0, 255), random(0, 255), random(0, 255)));
    }
    turns = random(0, (oldConnections + 1)) + randomNotes;
    SpeedIdx++; // Move to next position, ready for when this sample has completed
    period = WaitTime[random(0, 5)];
    pixels.show();
  }
  counter++;
}

// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________
// ___________________________________________________________________________________________________

void sendMessage()
{
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  mesh.sendBroadcast(msg);

  if (calc_delay)
  {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end())
    {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("Sending message: %s\n", msg.c_str());

  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5)); // between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String &msg)
{

  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end())
  {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}