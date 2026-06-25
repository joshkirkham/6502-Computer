/* Fill the AT28C256 EEPROM with Data */

#include "EepromImage.h"

char sprintfBuffer[1024];

/********** PIN ASSIGNMENTS **********/
//   PIN MAP =  A0, A1, A2, A3, A4, A5, ...
int ADDR[15] = {49, 48, 47, 46, 45, 44, 43, 42, 37, 36, 35, 34, 33, 32, 31};

//  PIN MAP =   D0, D1, D2, ...
int DATA[8] =  {53, 52, 51, 50, 10, 11, 12, 13};

int CE = 22;
int OE = 23;
int WE = 24;
int TRIGGER = 25; // Debug pin to trigger an oscope for debugging :)

/********** HELPERS **********/

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

void _WaitForWriteCycle(uint16_t addr, uint8_t expected) {
    int tries = 0;
    uint8_t value = ReadByte(addr);
    while (value != expected && tries < 50) {
      delay(1);
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

  baseAddr = baseAddr & (~0x3F);

  for (int i = 0; i < PAGE_SIZE; ++i) {
    uint16_t addr = baseAddr + i;
    _AssertAddress(addr);
    _AssertData((uint8_t)pgm_read_byte_near(data + i));
    digitalWrite(WE, 0);
    digitalWrite(WE, 1);
  }

  _WaitForWriteCycle(baseAddr + (PAGE_SIZE-1), (uint8_t)pgm_read_byte_near(data + PAGE_SIZE - 1));
}



int ValidatePage(uint16_t baseAddr, uint8_t* expectedData) {
  int numErrors = 0;
  baseAddr = baseAddr & (~0x3F);
  for (int i = 0; i < PAGE_SIZE; ++i) {
    uint8_t expected = pgm_read_byte_near(expectedData + i);
    uint8_t actual = ReadByte(baseAddr + i);

    if (actual != expected) {
      ++numErrors;
      
      snprintf(sprintfBuffer, 1024, "Page %04X  Byte %02X\tExpected %02X\tSaw %02X", baseAddr, i, expected, actual);
      Serial.println(sprintfBuffer);
      
    }
  }
  return numErrors;
}



/********* MAIN **********/

void setup() {
  Serial.begin(9600);
  Serial.println("\n========== AT28C256 Programming ==========");

  pinMode(CE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(WE, OUTPUT);
  digitalWrite(CE, 0);
  digitalWrite(OE, 1);
  digitalWrite(WE, 1);

  // First page fails to write without this explicitly set ahead of time. Why? Signals look good on oscope without it...
  _SetupBusForWrite();
  _AssertAddress(0);
  _AssertData(0);
  
  pinMode(TRIGGER, OUTPUT);
  digitalWrite(TRIGGER, 0);
  delay(500);



  // Manually program the reset vector.
  WriteByte(0x7FFD, 0x80);
  delay(20);
  WriteByte(0x7FFC, 0x00);
  delay(20);

  int numErrors = 0;
  int numPagesWithError = 0;

  for (int page = 0; page < NUM_PAGES; ++page) {

    // Skip programming the last page (511 / 0x7FC0)
    // For some reason it only works with the individual byte function
    // Why? idk... For now just fill the reset vector manually
    // TODO: Figure this out.
    if (page == NUM_PAGES - 1) { continue; }

    uint16_t pageAddress = page * PAGE_SIZE;
    uint8_t* pageData = EEPROM_PAGE(page);

    WritePage(pageAddress, pageData);
    int err = ValidatePage(pageAddress, pageData);
    delay(1); // Needed?

    if (err > 0) {
      numErrors += err;
      numPagesWithError++;
    }

    if (page % 50 == 0) {
      snprintf(sprintfBuffer, 1024, "Finished Page %d/%d", page, NUM_PAGES);
      Serial.println(sprintfBuffer);
    }

  }

  Serial.println("Programming Finished");
  if (numErrors > 0) {
    snprintf(sprintfBuffer, 1024, "%d Errors in %d Pages", numErrors, numPagesWithError);
    Serial.println(sprintfBuffer);
  }
  else {
    Serial.println("Verification Passed");
  }

}


void loop() {
  // put your main code here, to run repeatedly:
}