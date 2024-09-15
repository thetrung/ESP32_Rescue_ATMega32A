#include <Arduino.h>
//
// Rescue ATMega32A from ATMega8
//
// #define HFUSE 0x99  // Default for ATmega48/88/168, for others see
// #define LFUSE 0xE1  // 0x62	// http://www.engbedded.com/cgi-bin/fc.cgi

// SIGNAL :
#define _RDY 32    // -> ATMega32A/15-PD1
#define _OE 33     // -> ATMega32A/16-PD2
#define _WR 25     // -> ATMega32A/17-PD3
#define _BS1 26    // -> ATMega32A/18-PD4
#define _XA0 27    // -> ATMega32A/19-PD5
#define _XA1 14    // -> ATMega32A/20-PD6
#define _PAGEL 12  // -> ATMega32A/21-PD7
#define _BS2 13    // -> ATMega32A/40-PA0

// DATA :
const int _DATA[8] = {
  15, 2, 0, 4,
  16, 17, 5, 18
};  //-> ATMega32A/PORTB(PB7-0)

// POWER :
#define _RST 21  // -> ATMega32A/09-RESET(+12V)
#define _VCC 23  // -> ATMega32A/10-VCC

// PULSE :
#define _XTAL1 19  // -> ATMega32A/13-XTAL1

// CONTROL :
// #define	_BUTTON	(1<<1) // (?)


bool CMD_WRITE_FUSE[8] = { 0, 1, 0, 0, 0, 0, 0, 0 };
bool CMD_READ_FUSE[8] = { 0, 0, 0, 0, 0, 1, 0, 0 };

void sendcmd(bool command[]) {
  // PC4-SDA
  // PORTC |= _XA1;
  digitalWrite(_XA1, HIGH);

  // PB5-SCK & PB1-OC1A
  // PORTB &= ~(_XA0|_BS1);
  digitalWrite(_XA0, LOW);
  digitalWrite(_BS1, LOW);

  // PCINT [16..23]
  // PORTD = command;
  for (int i = 0; i < 8; i++) {
    digitalWrite(_DATA[i], command[i] == 0 ? LOW : HIGH);
  }
  //
  // Crystal pulse 1000-Hz
  //
  // PORTC |= _XTAL1;
  digitalWrite(_XTAL1, HIGH);
  delay(1);

  // PORTC &= ~(_XTAL1);
  digitalWrite(_XTAL1, LOW);
  delay(1);
}

// Default ATMega32A fuse bits :
bool HFUSE[] = { 1, 0, 0, 1, 1, 0, 0, 1 };
bool LFUSE[] = { 1, 1, 1, 0, 0, 0, 0, 1 };

void writefuse(bool fuse[], bool highbyte) {
  // Load Data-Low-Byte :
  // Bit n = 1 -> erase Fuse bit
  // Bit n = 0 -> program Fuse bit
  digitalWrite(_XA1, LOW);
  digitalWrite(_XA0, HIGH);
  delay(1);

  // PORTD = fuse;
  for (int i = 0; i < 8; i++) {
    digitalWrite(_DATA[i], fuse[i] ? HIGH : LOW);
  }

  // // Give XTAL a positive pulse :
  // PORTC |= _XTAL1;
  // digitalWrite(_XTAL1, HIGH);
  // delay(1);
  // digitalWrite(_XTAL1, LOW);
  // _delay_ms(1);
  // // Disable _XTAL1
  // PORTC &= ~(_XTAL1);

  // HFUSE | LFUSE -> PB1-OC1A:
  // if (highbyte)
  // 	PORTB |= _BS1; 	 // _BS1 -> 1
  // else
  // 	PORTB &= ~(_BS1);// _BS1 -> 0
  digitalWrite(_BS1, highbyte ? HIGH : LOW);
  digitalWrite(_BS2, LOW);

  // WR = Write FUSE (Active = LOW)
  // PORTB &= ~(_WR);	 // _WR -> 0
  digitalWrite(_WR, LOW);
  delay(1);  //ms

  // Wait for RDY/BSY -> HIGH
  // while (digitalRead(_RDY) == LOW) {
  //   delay(1);  //ms
  // }

  // PB2-SS/OC1B :
  // WR = Write FUSE (Disable = HIGH)
  // PORTB |= _WR;  // _WR -> 1
  // digitalWrite(_WR, HIGH);
  digitalWrite(_BS1, LOW);
  delay(1000);  //ms
}
void print_data_pins() {
  for (byte i = 0; i < 8; i++) {
    int read = digitalRead(_DATA[i]);
    Serial.print(read);
  }
}
void _get_fuse_data(int BS1, int BS2) {
  digitalWrite(_BS1, BS1);
  digitalWrite(_BS2, BS2);
  digitalWrite(_OE, LOW);
  print_data_pins();
  Serial.println("\n----------------");
}
void reset_data_pins() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(_DATA[i], LOW);
  }
  delay(1);
}
void read_fuse() {
  // Reset Data first :
  reset_data_pins();
  for (int i = 0; i < 8; i++) {
    pinMode(_DATA[i], INPUT);
  }
  // Read HFUSE :
  // OE  -> 0
  // BS2 -> 0
  // BS1 -> 0
  Serial.print("HFUSE: ");
  _get_fuse_data(1, 1);
  // Read LFUSE :
  // OE  -> 0
  // BS2 -> 1
  // BS1 -> 1
  Serial.print("LFUSE: ");
  _get_fuse_data(0, 0);
  // Read LOCK bits :
  // OE  -> 0
  // BS2 -> 0
  // BS1 -> 1
  Serial.print("LOCKB: ");
  _get_fuse_data(1, 0);
  // DONE. Reset pins.
  digitalWrite(_OE, HIGH);
  digitalWrite(_BS1, LOW);
  digitalWrite(_BS2, LOW);
  Serial.println("Done reading fuse bits.");
  delay(1000);
}

void setup() {

  // Signal :
  pinMode(_RDY, INPUT);
  pinMode(_OE, OUTPUT);
  pinMode(_WR, OUTPUT);
  pinMode(_BS1, OUTPUT);
  pinMode(_XA0, OUTPUT);
  pinMode(_XA1, OUTPUT);
  pinMode(_PAGEL, OUTPUT);
  pinMode(_BS2, OUTPUT);

  // DATA :
  for (int i = 0; i < 8; i++) pinMode(_DATA[i], OUTPUT);

  // POWER :
  pinMode(_VCC, OUTPUT);
  pinMode(_RST, OUTPUT);

  // PULSE :
  pinMode(_XTAL1, OUTPUT);

  // setup UART
  Serial.begin(9600);
}
void loop() {
  //   for (;;) {
  // while (PINC & _BUTTON) {}	// wait for button

  // ~ Initialization HVPP ~
  //
  // * Apply 4.5-5V & wait at least 100us :
  // => Set HIGH -> 10-VCC
  // => Set HIGH -> WR/Write_Pulse @ 17-PD3 (Active = LOW)
  // => Set HIGH -> OE/Output_Enable @ 16-PD2 (Active = LOW)
  //
  // PORTB |= _VCC|_WR|_OE;
  Serial.println("Initialize VCC/WR/OE...");
  digitalWrite(_VCC, HIGH);
  // digitalWrite(_WR, HIGH);
  // digitalWrite(_OE, HIGH);
  // Wait 1ms..
  delay(1);
  // _RST -> HIGH => Turn-Off 12V
  digitalWrite(_RST, HIGH);
  // * Toggle _XTAL1 at least 6 times ?
  // for (int i = 0; i < 6; i++) {
  //   digitalWrite(_XTAL1, HIGH);
  //   delay(150);
  //   digitalWrite(_XTAL1, LOW);
  //   delay(150);
  //   Serial.println("Toggled XTAL1.");
  // }
  //
  // ~ Enter Programming Mode ~
  //
  // * Set Prog_Enable = "0000" by
  // 	 Clear bits value of 4 pins in 2 PORTs :
  //   21-PD7-PAGEL
  //   20-PD6-XA1
  //   19-PD5-XA0
  //   18-PD4-BS1
  //
  // PORTC &= ~(_PAGEL|_XA1); // 21-PD7 | 20-PD6
  // PORTB &= ~(_XA0|_BS1);   // 19-PD5 | 18-PD4
  digitalWrite(_PAGEL, LOW);
  digitalWrite(_XA1, LOW);
  digitalWrite(_XA0, LOW);
  digitalWrite(_BS1, LOW);
  Serial.println("Reset PAGEL, XA1, XA0, BS1");
  //
  // Apply 11.5-12V -> RESET/pin 10-RST :
  // * Set 10-RST -> LOW (to Active)
  // PORTC &= ~(_RST);
  digitalWrite(_RST, LOW);
  delay(1);
  Serial.println("RESET -> LOW.\nEnabled HVPP(+12V).");
  delay(800);
  Serial.println("Enter Programming Mode.");
  //
  // ~ Read FUSE bit ~
  //
  Serial.println("Read FUSE bit :");
  sendcmd(CMD_READ_FUSE);
  read_fuse();
  reset_data_pins();
  delay(1000);
  Serial.println("Continue to write fuse ?");
  int prompt = 0;
  while (true) {
    //
    // wait for input ...
    //
    while (!Serial.available()) {};
    // check :
    prompt = toupper(Serial.read());
    if (prompt == 'A' || ' ') break;
  }
  //
  //
  // ~ Write FUSE bit ~
  //
  // * Use Command-Byte : 0b01000000 (Write Fuse Bits)
  //
  // * Fuse High Bits :
  // * Send Command => DATA-PORTB
  // sendcmd(0b01000000);
  Serial.println("Write HFUSE:");
  sendcmd(CMD_WRITE_FUSE);
  print_data_pins();
  Serial.println(" <-CMD_WRITE_FUSE.");
  // * Write HFUSE with BS1/BS2 = 1/0:
  writefuse(HFUSE, 1);
  print_data_pins();
  reset_data_pins();
  Serial.println(" <-High Fuse bits.");
  // * Give WR -> LOW
  // * Wait for RDY/BSY -> HIGH
  // * Set BS1 -> 0 => select low-data-byte.
  //
  // * Fuse Low Bits :
  // * Send Command => DATA-PORTB
  // sendcmd(0b01000000);
  Serial.println("\nWrite LFUSE..");
  sendcmd(CMD_WRITE_FUSE);
  print_data_pins();
  Serial.println("<-CMD_WRITE_FUSE.");
  // * Write LFUSE with BS1/BS2 = 0/0 :
  writefuse(LFUSE, 0);
  print_data_pins();
  Serial.println(" <-Low Fuse bits.");
  // * Give WR -> LOW
  // * Wait for RDY/BSY -> HIGH

  // allow button to be released
  delay(1000);

  // Exit programming mode :
  // Set 9-RST -> 1 => 12V-OFF
  // PORTC |= _RST;
  Serial.println("Exit Programming Mode..");
  digitalWrite(_RST, HIGH);
  Serial.println("_RST -> HIGH");

  // Turn off outputs
  Serial.println("Turn-off pins..");
  // PORTD = 0x00;
  // PORTB &= ~(_VCC | _WR | _OE | _XA0 | _BS1);
  // PORTC &= ~(_PAGEL | _XA1 | _BS2);
  digitalWrite(_VCC, LOW);
  digitalWrite(_WR, LOW);
  digitalWrite(_OE, LOW);
  digitalWrite(_XA0, LOW);
  digitalWrite(_XA1, LOW);
  digitalWrite(_BS1, LOW);
  digitalWrite(_BS2, LOW);
  digitalWrite(_PAGEL, LOW);
  for (int i = 0; i < 8; i++) digitalWrite(_DATA[i], LOW);
  // Reset Time...
  delay(5000);
  // }
}
