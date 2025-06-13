/********** PIN ASSIGNMENTS **********/
int ADDR[15] = {49, 48, 47, 46, 45, 44, 43, 42, 37, 36, 35, 34, 33, 32, 31};
int DATA[8] =  {53, 52, 51, 50, 10, 11, 12, 13};
int CE = 22;
int OE = 23;
int WE = 24;

int TRIGGER = 25;

/********** HELPERS **********/

void _configIo(int* pins, int width, int mode) {
  for (int i = 0; i < width; ++i) {
    pinMode(pins[i], mode);
  }
}

void _assertPins(int* pins, int width, uint16_t value) {
  for (int i = 0; i < width; ++i) {
    digitalWrite(pins[i], (value >> i) & 1);
  }
}

/*
#define _assertAddr(value) (assertPins(ADDR, 15, (value)));
#define _assertData(value) (assertPins(DATA, 8, (value)));
*/

void _assertAddr(uint16_t value) {
  PORTL = (uint8_t) (value & 0x00FF);
  PORTC = (uint8_t) ((value >> 8) & 0x00FF);
  /*
  char buffer[1024];
  sprintf(buffer, "%d -> %x %x / %x %x\r\n", value, value & 0x00FF, (value >> 8) & 0x00FF, PINL, PINC);
  Serial.println(buffer);
  */
}

void _assertData(uint8_t data) {
  PORTB = data;
}


uint16_t _readPins(int* pins, int width) {
  uint16_t value = 0;
  for (int i = 0; i < width; ++i) {
    value |= (digitalRead(pins[i]) << i);
  }
  return value;
}

void WaitForWriteCycle(uint16_t addr, uint8_t expected) {
    int tries = 0;
    uint8_t value;
    do {
      ++tries;
      value = ReadByte(addr);
      Serial.println(value);
      delay(100);
      
    } while (value != expected && tries < 50);
}

void WriteByte(uint16_t addr, uint8_t byte) {
  digitalWrite(WE, 1);
  digitalWrite(OE, 1);
  digitalWrite(CE, 1);
  _assertPins(ADDR, 15, addr);
  _configIo(DATA, 8, OUTPUT);
  _assertPins(DATA, 8, byte);
  digitalWrite(CE, 0);
  digitalWrite(WE, 0);
  //delay(100);
  digitalWrite(WE, 1);
  digitalWrite(CE, 1);
  //delay(100);
  digitalWrite(OE, 0);
}


void WritePage(uint16_t baseAddr, uint8_t* data) {
  digitalWrite(OE, 1);
  digitalWrite(WE, 1);
  digitalWrite(CE, 1);
  _configIo(ADDR, 15, OUTPUT);
  _configIo(DATA, 8, OUTPUT);


  digitalWrite(TRIGGER, 1);
  for (int i = 0; i < 64; ++i) {
    uint16_t addr = baseAddr + i;
    digitalWrite(CE, 1);
    digitalWrite(WE, 1);
    _assertAddr(addr);
    _assertData(data[i]);
    //_assertPins(ADDR, 15, addr);
    //_assertPins(DATA, 8, data[i]);
    digitalWrite(CE, 0);
    digitalWrite(WE, 0);
  }
  digitalWrite(WE, 1);
  digitalWrite(CE, 1);
  digitalWrite(OE, 0);

  WaitForWriteCycle(baseAddr + 63, data[63]);
}


uint8_t ReadByte(uint16_t addr) {
  _assertPins(ADDR, 15, addr);
  _configIo(DATA, 8, INPUT);
  digitalWrite(CE, 0);
  digitalWrite(OE, 0);
  //delay(100);
  uint8_t value =  _readPins(DATA, 8);
  digitalWrite(OE, 1);
  digitalWrite(CE, 1);
  //delay(100);
  return value;
}



/********* MAIN **********/

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("========== AT28C256 Programming (write disable) ==========");
  pinMode(CE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(WE, OUTPUT);
  digitalWrite(CE, 1);
  digitalWrite(OE, 1);
  digitalWrite(WE, 1);
  _configIo(ADDR, 15, OUTPUT);
  _configIo(DATA, 8, OUTPUT);


  pinMode(TRIGGER, OUTPUT);
  digitalWrite(TRIGGER, 0);


  char buffer[1024];
  uint8_t data[64];
  memset(data, 0xEA, 64);
  Serial.println("Writing Data");
  WritePage(0x00, data);
  Serial.println("Verifying");
  for (int i = 0; i < 64; ++i) {
    uint8_t value = ReadByte(i);
    Serial.println(value);
  }

/*
  uint8_t muhData[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBE};
  for (uint16_t addr = 0; addr < 6; ++addr) {
    WriteByte(addr, muhData[addr]);

    uint8_t value;
    unsigned long start = micros();
    int tries = 0;
    do {
      ++tries;
      value = ReadByte(addr);
      //snprintf(buffer, 1024, "%04x: %02x", addr, value);
      
    } while (value != muhData[addr]);

    unsigned long stop = micros();
    snprintf(buffer, 1024, "Write: %d tries, %ul us", tries, stop - start);
    Serial.println(buffer);


    //delay(100);
    //uint8_t value = ReadByte(addr);
    snprintf(buffer, 1024, "%04x: %02x", addr, value);
    Serial.println(buffer);
  }
  */

  return;

int correct = 0;
  for (int i = 0; i < 65536; ++i) {
    if (i % 1024 == 0) {
      Serial.println(i);
        snprintf(buffer, 1024, "%d Correct: %d / 65536", i, correct);
        Serial.println(buffer);
    }
    
    WriteByte(i, 0xEA);
    delay(1);
    uint8_t value = ReadByte(i);
    Serial.println(value);
    if (value == 0xEA) { ++ correct; }

  }


  //digitalWrite(CE, 1);

  /*
  configIo(ADDR, 15, OUTPUT);
  assertPins(ADDR, 15, 0xAA);
  configIo(DATA, 8, OUTPUT);
  assertPins(DATA, 8, 0xFF);
  */

}

void loop() {
  // put your main code here, to run repeatedly:

}
