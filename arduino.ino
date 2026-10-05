#include <Arduino.h>


int selectedLED = -1;
int inputValue = 0;
bool inputStarted = false;


unsigned long blinkRateOne = 0;
unsigned long blinkRateTwo = 0;


unsigned long blinkTrackerOne = 0;
unsigned long blinkTrackerTwo = 0;


// Custom mymilli counter
volatile unsigned long myMilliCounter = 0;


// Custom register definitions
#define MY_TCCR2A (*(volatile uint8_t *)0xB0)
#define MY_TCCR2B (*(volatile uint8_t *)0xB1)
#define MY_TCNT2  (*(volatile uint8_t *)0xB2)
#define MY_OCR2A  (*(volatile uint8_t *)0xB3)
#define MY_TIMSK2 (*(volatile uint8_t *)0x70)
#define MY_SREG (*(volatile uint8_t *)0x5F)
#define INTERRUPT_BIT 7


// Task prototypes
void taskLED1();
void taskLED2();
void taskSerial();




// Function pointer table
void (*tasks[])() = {
  taskLED1,
  taskLED2,
  taskSerial
};


// Custom timer interrupt
ISR(TIMER2_COMPA_vect) {
  myMilliCounter++;
}


// Millis replacement
unsigned long mymilli() {

  unsigned char oldStatus;
  unsigned long currentTime;

  oldStatus = MY_SREG;

  // Disable interrupts
  MY_SREG &= ~(1 << INTERRUPT_BIT);

  currentTime = myMilliCounter;

  // Restore previous interrupt state
  MY_SREG = oldStatus;

  return currentTime;
}


// Setup mymilli timer
void setupMyMilli() {

  MY_TCCR2A = 0;
  MY_TCCR2B = 0;
  MY_TCNT2 = 0;

  // 1 millisecond interrupt
  MY_OCR2A = 249;

  // CTC mode
  MY_TCCR2A |= (1 << 1);

  // Prescaler = 64
  MY_TCCR2B |= (1 << 2);

  // Enable compare interrupt
  MY_TIMSK2 |= (1 << 1);
}




void setup() {
  // calls all the functions needed to start
  setupMyMilli();

  Serial.begin(9600);


  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  Serial.println("What LED? (1 or 2):");
}




void loop() {


  // Round-robin cyclic executive
  for (int i = 0; i < 3; i++) {
    tasks[i]();
  }
}




void taskLED1() {


  unsigned long currentTime = mymilli();


  if (blinkRateOne &&
      (currentTime - blinkTrackerOne) >= (blinkRateOne / 2)) {


    digitalWrite(2, !digitalRead(2));
    blinkTrackerOne = currentTime;
  }
}




void taskLED2() {


  unsigned long currentTime = mymilli();


  if (blinkRateTwo &&
      (currentTime - blinkTrackerTwo) >= (blinkRateTwo / 2)) {


    digitalWrite(3, !digitalRead(3));
    blinkTrackerTwo = currentTime;
  }
}




void taskSerial() {


  if (Serial.available() > 0) {


    char input = Serial.read();


    // If input is a number
    if (input >= '0' && input <= '9') {


      inputValue = inputValue * 10 + (input - '0');
      inputStarted = true;
    }


    // If user presses Enter
    else if ((input == '\n' || input == '\r') && inputStarted) {


      // Getting LED number
      if (selectedLED == -1) {


        selectedLED = inputValue;
        Serial.println("What interval (in msec):");
      }


      // Getting blink interval
      else {


        if (selectedLED == 1) {
          blinkRateOne = inputValue;
        }


        else if (selectedLED == 2) {
          blinkRateTwo = inputValue;
        }


        selectedLED = -1;
        Serial.println("What LED? (1 or 2):");
      }


      inputValue = 0;
      inputStarted = false;
    }
  }
}
