// Monitor the Address & Data Busses of the 6502

#include <inttypes.h>

/********** Pin Definitions **********/
#define CLOCK_PIN 21
#define DATA_BUS_LSB_PIN  22 // Data Pins 22 - 29
#define ADDR_BUS_LSB_PIN 32 // Addr Pins 32-47
#define RWB_PIN 52


/********** Helper Functions **********/
void setModeRange(int pinStart, int width, int mode) {
  for (int i = 0; i < width; ++i) {
    pinMode(pinStart + i, mode);
  }
}

int readRange(int pinStart, int width) {
  int data = 0;
  for (int i = 0; i < width; ++i) {
    data |= digitalRead(pinStart + i) << i;
  }
  return data;
}

void toBitString(int x, char* out, int len) {
  for (int i = 0; i < len; ++i) {
    out[i] = (x & (1<<i))? '1' : '0';
  }
  out[len] = '\0';
}


/********** Main Program **********/
bool shouldRead;
char outputBuffer[1024];
bool activityLedState = true;

// Interrupt handler toggles flag to read bus on every rising clock
void onClockEdge() {
  shouldRead = true;
}

// Runs Once During Startup
void setup() {
  Serial.begin(9600);
  Serial.println("========== RESTART BUS MONITOR ==========");
  Serial.println("Address          Data     RW Addr Data");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CLOCK_PIN, INPUT);
  setModeRange(ADDR_BUS_LSB_PIN, 16, INPUT);
  setModeRange(DATA_BUS_LSB_PIN, 8, INPUT);
  attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), onClockEdge, RISING);

  memset(outputBuffer, 0, 1024);
  shouldRead = true; 
}

// Continuously Monitor Bus Lines
void loop() {

  if (shouldRead) {
    char addrBits[17];
    char dataBits[9];
    int addr = readRange(ADDR_BUS_LSB_PIN, 16);
    int data = readRange(DATA_BUS_LSB_PIN, 8);
    char readWrite = digitalRead(RWB_PIN)? 'R':'W';
    
    digitalWrite(LED_BUILTIN, activityLedState);
    activityLedState = !activityLedState;

    toBitString(addr, addrBits, 16);
    toBitString(data, dataBits, 8);
    snprintf(outputBuffer, 1024, "%s %s  %c  %04x %02x", addrBits, dataBits, readWrite, addr, data);
    Serial.println(outputBuffer);

    shouldRead = false;
  }
  
  delay(100);

}
