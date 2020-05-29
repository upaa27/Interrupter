/* 
* Monophonic MIDI DRSSTC Interrupter 
* Hardware: 
*  - Port D.2 (Arduino Digital Pin 2) to interrupter input
*  - Takes MIDI from hardware or virtual serial
*/ 

#include <MIDI.h> 
MIDI_CREATE_DEFAULT_INSTANCE();

void setupTimers(); 
void setupInterrupts(); 
void noteOnHandler(byte, byte, byte); 
void noteOffHandler(byte, byte, byte);
int  findOnTime(int); 

// Global constants 
#define PW_TABLE_MULTIPLIER  2.4 

// Global primitives 
volatile int on_time, current_pitch = 0; 

void setup() { 
  DDRD  |=  (1 << 2); 
  PORTD &= ~(1 << 2);    // Start the pin LOW
   
  setupTimers(); 
  setupInterrupts(); 
   
  // Initiate MIDI communications, listen to all channels 
  MIDI.begin(MIDI_CHANNEL_OMNI);     
  Serial.begin(115200);

  // Setup MIDI callback functions (add more callback functions here to handle other MIDI events) 
  MIDI.setHandleNoteOn(noteOnHandler);
  MIDI.setHandleNoteOff(noteOffHandler);
} 

void setupTimers() { 
  TCCR1B = (1 << CS11) | (1 << WGM12);  // CTC mode, set prescaler to divide by 8 for 1 / 2MHz or 0.5uS per tick (clkio = 16MHz) 
} 

void setupInterrupts() { 
  sei();    // Enable global interrupts
} 

// Main program loop polls for MIDI events 
void loop() { 
  MIDI.read(); 
} 

// MIDI callback functions 
void noteOnHandler(byte channel, byte pitch, byte velocity) { 
  if (channel == 1) { 
    TIMSK1 &= ~(1 << OCIE1A);    // Disable Timer1's overflow interrupt to halt the output 
     
    current_pitch = pitch;  // Store the pitch that will be played 
     
    int frequency = (int) (110.0 * pow(pow(2.0, 1.0/12.0), pitch - 57) + 0.5);  // Decypher the pitch number 
    int period = 1000000 / frequency;                                           // Perform period and calculation 
    on_time = findOnTime(frequency);                                            // Get a value from the pulsewidth lookup table 
     
    OCR1A   = 2 * period;     // Set the compare value 
    TCNT1   = 0;              // Reset Timer1 
    TIMSK1 |= (1 << OCIE1A);  // Enable the compare interrupt (start playing the note) 
  } 

}

void noteOffHandler(byte channel, byte pitch, byte velocity) {
  if (velocity == 0 && channel == 1)
    TIMSK1 &= ~(1 << OCIE1A);
}

// Pulsewidth lookup table 
int findOnTime(int frequency_input) { 
  int on_time = 17; 
  if (frequency_input < 1000) {on_time = 17;} 
  if (frequency_input < 900)  {on_time = 18;}   
  if (frequency_input < 800)  {on_time = 20;} 
  if (frequency_input < 700)  {on_time = 20;} 
  if (frequency_input < 600)  {on_time = 23;} 
  if (frequency_input < 500)  {on_time = 27;} 
  if (frequency_input < 400)  {on_time = 30;}   
  if (frequency_input < 300)  {on_time = 35;} 
  if (frequency_input < 200)  {on_time = 40;} 
  if (frequency_input < 100)  {on_time = 45;}        
  on_time *= PW_TABLE_MULTIPLIER; 
  return on_time; 
} 

// Interrupt vectors 
ISR (TIMER1_COMPA_vect) { 
  PORTD |= (1 << 2);           // Set the optical transmit pin high 
  delayMicroseconds(on_time);  // Wait 
  PORTD &= ~(1 << 2);          // Set the optical transmit pin low 
} 
