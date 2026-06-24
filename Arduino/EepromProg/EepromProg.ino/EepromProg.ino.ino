/********** PIN ASSIGNMENTS **********/

//   PIN MAP =  A0, A1, A2, A3, A4, A5, ...
int ADDR[15] = {49, 48, 47, 46, 45, 44, 43, 42, 37, 36, 35, 34, 33, 32, 31};

//  PIN MAP =   D0, D1, D2, ...
int DATA[8] =  {53, 52, 51, 50, 10, 11, 12, 13};

int CE = 22;
int OE = 23;
int WE = 24;

int TRIGGER = 25;

/********** HELPERS **********/

#define PAGE_SIZE 64

/*
#define _assertAddr(value) (assertPins(ADDR, 15, (value)));
#define _assertData(value) (assertPins(DATA, 8, (value)));
*/

void _AssertAddress(uint16_t value) {
  PORTL = (uint8_t) (value & 0x00FF);
  PORTC = (uint8_t) ((value >> 8) & 0x00FF);
}

void _AssertData(uint8_t data) {
  PORTB = data;
}

uint8_t _ReadData(void) {
  return PINB;
}

void _SetupBusForRead(void) {
  DDRL = 0xFF; // Address = Output
  DDRC = 0xFF;
  DDRB = 0x00; // Data = Input
}

void _SetupBusForWrite(void) {
  DDRL = 0xFF; // Address = Output
  DDRC = 0xFF;
  DDRB = 0xFF; // Data = Output
}

void _SetBusEnables(int state) {
  digitalWrite(CE, state);
  digitalWrite(OE, state);
}

void _WaitForWriteCycle(uint16_t addr, uint8_t expected) {
    int tries = 0;
    uint8_t value = ReadByte(addr);
    while (value != expected && tries < 50) {
      delay(100);
      value = ReadByte(addr);
      ++tries;
    } 

    if (value != expected) {
      Serial.println("ERROR: Timed out waiting for write cycle.");
    }
}


uint8_t ReadByte(uint16_t addr) {
  _SetupBusForRead();
  digitalWrite(WE, 1);
  _AssertAddress(addr);

  digitalWrite(OE, 0);
  uint8_t value = _ReadData();
  digitalWrite(OE, 1);
  return value;
}

void WriteByte(uint16_t addr, uint8_t data) {
  _SetupBusForWrite();
  digitalWrite(OE, 1);
  _AssertAddress(addr);
  _AssertData(data);

  digitalWrite(WE, 0);
  digitalWrite(WE, 1);
}


void WritePage(uint16_t baseAddr, uint8_t* data) {
  _SetupBusForWrite();
  digitalWrite(OE, 1);
  digitalWrite(TRIGGER, 1);

  baseAddr = baseAddr & (~0x3F);

  for (int i = 0; i < PAGE_SIZE; ++i) {
    uint16_t addr = baseAddr + i;
    _AssertAddress(addr);
    _AssertData(data[i]);
    digitalWrite(WE, 0);
    digitalWrite(WE, 1);
  }

  _WaitForWriteCycle(baseAddr + 63, data[63]);
}

int ValidatePage(uint16_t baseAddr, uint8_t* expectedData) {
  int numErrors = 0;
  baseAddr = baseAddr & (~0x3F);
  for (int i = 0; i < PAGE_SIZE; ++i) {
    if (ReadByte(baseAddr + i) != expectedData[i]) {
      ++numErrors;
    }
  }
  return numErrors;
}






/********* MAIN **********/

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("========== AT28C256 Programming (write disable) ==========");
  pinMode(CE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(WE, OUTPUT);
  digitalWrite(CE, 0);
  digitalWrite(OE, 1);
  digitalWrite(WE, 1);

  pinMode(TRIGGER, OUTPUT);
  digitalWrite(TRIGGER, 0);


  char buffer[1024];
  uint8_t data[64];
  memset(data, 0xEA, 64);

  Serial.println("Writing Data");
  for (int page = 0; page < 512; ++page) {
    uint16_t pageAddress = page * PAGE_SIZE;
    WritePage(pageAddress, data);
    int numErrors = ValidatePage(0x00, data);
    if (numErrors > 0) {
      snprintf(buffer, 1024, "Page @ %x: ERROR (%d)", pageAddress, numErrors);
    }
    else {
      snprintf(buffer, 1024, "Page @ %x: OKAY", pageAddress);
    }
    Serial.println(buffer);
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



}

void loop() {
  // put your main code here, to run repeatedly:

}



// SLOW FUNCTIONS
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


uint16_t _readPins(int* pins, int width) {
  uint16_t value = 0;
  for (int i = 0; i < width; ++i) {
    value |= (digitalRead(pins[i]) << i);
  }
  return value;
}
