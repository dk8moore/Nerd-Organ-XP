#include <Arduino.h>
#include <types.h>
#include <MIDI.h>

#define NUM_BANKS         8   // number of group of keys (for Fatar 61 they're arranged in 8 banks)
#define NUM_KEYS         61   // number of keys in the keyboard used

bank_t banks[NUM_BANKS];
bank_t prev_banks[NUM_BANKS];
key_fatar_t keys[NUM_KEYS];         // number of keys of the keybed (Fatar 61)

byte ctrlPins[] = {7, 8, 10, 11, 12};                    // bits 17, 16, 0, 2, 1 of GPIO2 (GPIO7) respectively, used to address scan (mplx)
byte inPins[] = {14, 15, 16, 17, 18, 19, 20, 21};      // bits 18, 19, 23, 22, 17, 16, 26, 27 of GPIO1 (GPIO6) respectively, used to read input from grid matrix
int inBits[] = {18, 19, 23, 22, 17, 16, 26, 27};

byte left, right;     // read word from keyboard: pins are connected LEFT: BRBRBRBR MKMKMKMK, RIGHT: BRBRBRBR MKMKMKMK

byte highTrig = 0;    // 0 if in setup mode, 1 if in High Trigger mode, 2 if in Normal Mode

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
int midiCh = 6;

/*
#define CPU_RESET_CYCLECOUNTER    do { ARM_DEMCR |= ARM_DEMCR_TRCENA;          \
                                       ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA; \
                                       ARM_DWT_CYCCNT = 0; } while(0)
volatile int cycles;
*/

void setup_scan();
void scan();
void trigger(key_fatar_t *key, event_t event);
void increment();
int velocity(int t);

void setup() {

   // Start serial comm for debug purposes
   Serial.begin(9600);

   // Setup output pins
   GPIO7_GDIR |= (0x30007);

   // Setup input pins (pulldown resistors - default)
   GPIO6_GDIR &= ~(0xCCF0000);
   
   // Initialize banks array at 0
   memset(banks, 0x00, sizeof(banks));
   memset(prev_banks, 0x00, sizeof(prev_banks));
   
   // Initialize keyboard keys struct
   for (int key=0; key<61; key++) {
      keys[key].midi_note = 36 + key;
      keys[key].t = 0;
      keys[key].played = false;
   }
   
   // while(highTrig==0) {
   //    setup_scan(); 
   // }
   
   MIDI.begin();
}

// MAIN LOOP
void loop() {
   scan();
   // increment(); 
}

// Before entering the playing mode, there's an initial loop to select the trigger mode of keys and the midi channel (optional)
// Trigger mode: NORMAL on key C2 (36), HIGH-TRIG (Hammond mode) on key C7 (96)
// MIDI Channel: C4=1, C#4=2, D4=3 ... D#5=16
// void setup_scan() {
//    for(int bank=0; bank<NUM_BANKS; bank++) {
//       prev_banks[bank] = banks[bank]; // Store previous state so we can look for changes
//       // Scan left keyboard

//       GPIOC_PDOR = (bank & 0xF) | 8;
//       delayMicroseconds(10); // Debounce
//       left = GPIOD_PDIR & 0xF;  // Read only the break signals
     
//       // Scan right keyboard
//       GPIOC_PDOR = (bank & 0x7) | 16;
//       delayMicroseconds(10); // Debounce
//       right = GPIOD_PDIR & 0xF;  // Read only the break signals
//       banks[bank].breaks = left | (right<<4);
//    }
  
//    // Check C2 for NORMAL MODE
//    if(banks[0].breaks & 0x1) {
//       Serial.println("Normal mode selected..");
//       Serial.print("MIDI channel: ");
//       Serial.println(midiCh);
//       highTrig = 2;
//       return;
//    }
//    // Check C7 for HIGH-TRIG MODE
//    if(banks[4].breaks & 0x80) {
//       Serial.println("High trigger mode selected..");
//       Serial.print("MIDI channel: ");
//       Serial.println(midiCh);
//       highTrig = 1;
//       return;
//    }
   
//    byte diff;
   
//    for(int bank=0; bank<NUM_BANKS; bank++) {
//       diff = (banks[bank].breaks ^ prev_banks[bank].breaks) & banks[bank].breaks;
//       switch(diff) {
        
//          case 0x8:
//             midiCh = bank+1;
//             /*
//             Serial.print("New MIDI channel: ");
//             Serial.println(midiCh);
//             */
//          break;

//          case 0x10:
//             midiCh = bank+9;
//             /*
//             Serial.print("New MIDI channel: ");
//             Serial.println(midiCh);
//             */
//          break;

//          case 0x1:
//          case 0x2:
//          case 0x4:
//          case 0x20:
//          case 0x40:
//          case 0x80:
//             /*
//             Serial.print("Unassigned key. MIDI channel is: ");
//             Serial.println(midiCh);
//             */
//          break;
         
//          default:
//          break;
//       }
      
//    }
  
// }

// Scan all the keys divided in banks
void scan() {
   
   // Scan and store
   for(int bank=0; bank<NUM_BANKS; bank++) {
      prev_banks[bank] = banks[bank]; // Store previous state so we can look for changes
      // Scan left keyboard
      Serial.println('Accendo i LED left');
      GPIO7_DR_SET = (1 << 16) | (bank & 0x7);
      delayMicroseconds(10); // Debounce
      left = (GPIO6_DR >> 16);
      delay(1000);
      GPIO7_DR_CLEAR = (1 << 16);

     
      // Scan right keyboard
      Serial.println('Accendo i LED right');
      GPIO7_DR_SET = (1 << 17) | (bank & 0x7);
      delayMicroseconds(10); // Debounce
      right = (GPIO6_DR >> 16);
      delay(1000);

      // banks[bank].breaks = (left & 0x0F) | (right & 0x0F)<<4;
      // banks[bank].makes = (left & 0xF0)>>4 | (right & 0xF0);

      Serial.println('Spengo tutto');
      GPIO7_DR_CLEAR = (0x30007);
      delay(1000);
   }
   // Process
   // for(int bank=0; bank<NUM_BANKS; bank++) {
   //    byte diff;
      
   //    // Check top switches and fire events
   //    diff = banks[bank].breaks ^ prev_banks[bank].breaks;
   //    if(diff) {
   //       //Serial.println(bank);
   //       for(int key=0; key<8; key++) {
   //          if(bitRead(diff, key)) { 
   //             event_t event = bitRead(banks[bank].breaks, key) ? KEY_TOUCHED : KEY_TOP;
   //             trigger(&keys[bank+key*8], event);
   //          }
   //       }
   //    }

   //    // Check bottom switches and fire events
   //    diff = banks[bank].makes ^ prev_banks[bank].makes;
   //    if(diff) {
   //       for(int key=0; key<8; key++) {
   //          if(bitRead(diff, key)) { 
   //             event_t event = bitRead(banks[bank].makes, key) ? KEY_PRESSED : KEY_RELEASED;           
   //             trigger(&keys[bank+key*8], event);
   //          }
   //       }
   //    }
   // }
}

byte compactDrRead(word dr, int *arrangement[]) {
   byte out = 0x00;
   for(int i=0; i<sizeof(arrangement); i++) {
      out |= (dr & (1 << (int(arrangement[i] - 16)))) >> int((arrangement[i] - 16 + i));
   }
   return out;
}

// Send a MIDI message of note ON or note OFF if the key reach top or bottom, change the states of the keys
void trigger(key_fatar_t *key, event_t event) {

   switch (event) {
      case KEY_TOUCHED:
         if(highTrig==1) {
          /*
            MIDI.sendNoteOn(key->midi_note, 1, midiCh);
            Serial.print("MIDI ON: ");
            Serial.print(key->midi_note);
            Serial.print(" V: ");
            Serial.println(1);
            */
            key->state = KEY_IS_DOWN;
            key->played = true;
            return;
         }
         key->state = KEY_IS_GOING_DOWN;
         key->t = 0;
      break;

      case KEY_PRESSED:
         if(key->played == false && highTrig==2) {
            /*
            Serial.print("MIDI ON: ");
            Serial.print(key->midi_note);
            Serial.print(" V: ");
            Serial.println(velocity(key->t));
            */
            MIDI.sendNoteOn(key->midi_note, velocity(key->t), midiCh);
            
            key->state = KEY_IS_DOWN;
            key->played = true;
         }
         key->t = 0;
      break;

      case KEY_RELEASED:
         key->state = KEY_IS_GOING_UP;
         key->t = 0;
      break;

      case KEY_TOP:
         if(key->played == true) {
            /*
            Serial.print("MIDI OFF: ");
            Serial.print(key->midi_note);
            Serial.print(" V: ");
            Serial.println(velocity(key->t));
            */
            MIDI.sendNoteOff(key->midi_note, velocity(key->t), midiCh);

            key->state = KEY_IS_UP;
            key->played = false;
         }
         key->t = 0;  
      break;

      default:
      break;
  }
}

// Increment the t values of each touched or released key
void increment() {

   for(int key=0; key<NUM_KEYS; key++) {
      state_t state = keys[key].state;
      if(state == KEY_IS_GOING_UP || state == KEY_IS_GOING_DOWN) {
         keys[key].t++;
      }
   }
}

// Compute the velocity from the time through a piecewise linear function
// ####################### NEED SOME TUNING #############################
int velocity(int t) {
   if(t<11)
      return 127;
   if(t<28)
      return int(-1.588*t + 142.882);
   if(t<41)
      return int(-1.5385*t + 141.5385);
   if(t<67)
      return int(-0.7692*t + 110.769);
   if(t<111)
      return int(-0.4545*t + 90);
   if(t<181)
      return int(-0.2857*t + 71.4286);
   if(t<330)
      return int(-0.1267*t + 42.8);
   if(t>329)
      return 1;
}
