#include <inttypes.h>

#define RWB_PIN 52
#define CLOCK_PIN 2
#define ADDR_BUS_LSB_PIN 32
#define DATA_BUS_LSB_PIN  22


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

bool BEGIN_READ = false;
void onClockEdge() {
  BEGIN_READ = true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("========== RESTART BUS MONITOR ==========");
  Serial.println("Address          Data     RW Addr Data");
  pinMode(LED_BUILTIN, OUTPUT);
  setModeRange(ADDR_BUS_LSB_PIN, 16, INPUT);
  setModeRange(DATA_BUS_LSB_PIN, 8, INPUT);
  pinMode(CLOCK_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), onClockEdge, RISING);
  BEGIN_READ = true; 
}

char outputBuffer[1024];
uint8_t activityLed = 1;

void loop() {
  if (BEGIN_READ) {
    char addrBits[17];
    char dataBits[9];
    int addr = readRange(ADDR_BUS_LSB_PIN, 16);
    int data = readRange(DATA_BUS_LSB_PIN, 8);
    char readWrite = digitalRead(RWB_PIN)? 'R':'W';
    
    digitalWrite(LED_BUILTIN, activityLed);
    activityLed = (activityLed + 1) % 2;

    toBitString(addr, addrBits, 16);
    toBitString(data, dataBits, 8);
    snprintf(outputBuffer, 1024, "%s %s  %c  %04x %02x", addrBits, dataBits, readWrite, addr, data);
    Serial.println(outputBuffer);

    BEGIN_READ = false;
  }
  delay(100);

}
