/*
 * PROJECT: AIR ARP - FIXED BUTTON LOGIC (Old Code Style)
 * HARDWARE: Teensy 3.2 / 4.0 / 4.1
 * PINS: Button(4), Trig(2), Echo(3), Knob(A0), Flex(A1)
 */

// --- PINS ---

const int PIN_BUTTON = 4;
const int PIN_TRIG   = 2;
const int PIN_ECHO   = 3;
const int PIN_KNOB   = A0;
const int PIN_FLEX   = A1;

// --- SETTINGS ---

const int MIDI_CHANNEL     = 1;
const int NOTE_VELOCITY    = 64;
const int MAX_DISTANCE_CM  = 70;

// --- MUSICAL SCALES ---

const int NUM_SCALES = 4;
int scales[NUM_SCALES][8] = {
  // 0: Major Pentatonic (1, 2, 3, 5, 6)
  { 0, 2, 4, 7, 9, 12, 14, 16 },
  // 1: Minor Pentatonic (1, b3, 4, 5, b7)
  { 0, 3, 5, 7, 10, 12, 15, 17 },
  // 2: Blues Scale (minor blues: 1, b3, 4, b5, 5, b7)
  { 0, 3, 5, 6, 7, 10, 12, 15 },
  // 3: Messiaen Mode 2 (octatonic: 0, 1, 3, 4, 6, 7, 9, 10)
  { 0, 1, 3, 4, 6, 7, 9, 10 }
};

// GLOBAL VARIABLES

int currentScaleIndex = 0;
int rootNote          = 60;
int arpMode           = 0;

// INTERNAL VARIABLES

unsigned long lastStepTime = 0;
int           currentStep  = 0;
int           direction    = 1;
int           activeNote   = -1;
unsigned long speedMillis  = 0;
int           gatePercent  = 50;

// BUTTON STATE (INPUT_PULLUP, active LOW)

int           lastButtonState   = HIGH;  // raw state
int           stableButtonState = HIGH;  // debounced state
unsigned long buttonPressTime   = 0;
bool          buttonHeld        = false;
bool          shiftModeActive   = false;
unsigned long lastChangeTime    = 0;
const unsigned long debounceMs  = 20;

// DEBUG TIMER

unsigned long lastDebugPrint = 0;

void setup() {
  // Button: to GND, use internal pull‑up
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis();
  int noteVelocity = 65;
  // 1. INPUTS
  // ---------------------------------------------

  // --- BUTTON LOGIC (INPUT_PULLUP + debounce, active LOW) ---
  int rawButton = digitalRead(PIN_BUTTON);  // HIGH = released, LOW = pressed

  // detect raw state change
  if (rawButton != lastButtonState) {
    lastChangeTime   = now;
    lastButtonState  = rawButton;
  }

  // after debounceMs, consider state stable
  if (now - lastChangeTime > debounceMs) {
    if (stableButtonState != rawButton) {
      stableButtonState = rawButton;

      // pressed (LOW)
      if (stableButtonState == LOW) {
        buttonPressTime = now;
        buttonHeld      = true;
        shiftModeActive = false;  // wait for hold

        // change mode ONLY once per stable press
        arpMode++;
        if (arpMode > 3) arpMode = 0;

        currentStep = 0;  // reset pattern
      }
      // released (HIGH)
      else {
        buttonPressTime = 0;
        buttonHeld      = false;
        shiftModeActive = false;
      }
    }
  }

  // Hold detection (>300 ms)
  if (buttonHeld && (now - buttonPressTime > 300)) {
    shiftModeActive = true;
  }

  // --- KNOB LOGIC ---
  int knobValue = analogRead(PIN_KNOB);

  if (shiftModeActive) {
    // MODE: SHIFT PITCH (Root Note)
    int newRoot = map(knobValue, 0, 1024, 48, 84);

    if (newRoot != rootNote) {
      rootNote = newRoot;
      if (activeNote != -1) usbMIDI.sendNoteOff(activeNote, 0, MIDI_CHANNEL);
    }
  } else {
    // MODE: GATE LENGTH
    gatePercent = map(knobValue, 0, 1024, 10, 150);
  }

  // --- SENSORS (Flex & Ultrasonic) ---

  int rawFlex = analogRead(PIN_FLEX);
  if (rawFlex > 920) rawFlex = 920;

  // mapping flex -> velocity (adjust ranges for your sensor)
  noteVelocity = map(rawFlex, 844, 920, 100, 10);
  noteVelocity = constrain(noteVelocity, 1, 127);

  // int rawFlex = analogRead(PIN_FLEX);
  // if (rawFlex > 920) rawFlex = 920;

  int bendValue = map(rawFlex, 920, 844, 8192, 16383);
  bendValue     = constrain(bendValue, 0, 16383);
  // usbMIDI.sendPitchBend(bendValue, MIDI_CHANNEL);

  // Ultrasonic Speed
  long duration, cm;

  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  duration = pulseIn(PIN_ECHO, HIGH, 6000);

  if (duration == 0) cm = 999;
  else               cm = duration / 29 / 2;

  if (cm > MAX_DISTANCE_CM) {
    speedMillis = 0;
  } else {
    speedMillis = map(cm, 2, MAX_DISTANCE_CM, 50, 800);
  }

  // 2. ARPEGGIATOR ENGINE
  // ---------------------------------------------
  if (speedMillis > 0) {
    unsigned long currentMillis = now;

    // time for next note?
    if (currentMillis - lastStepTime >= speedMillis) {
      lastStepTime = currentMillis;

      if (activeNote != -1) usbMIDI.sendNoteOff(activeNote, 0, MIDI_CHANNEL);

      // pattern
      if (arpMode == 0) {
        currentStep++;
        if (currentStep >= 8) currentStep = 0;
      } else if (arpMode == 1) {
        currentStep--;
        if (currentStep < 0) currentStep = 7;
      } else if (arpMode == 2) {
        currentStep += direction;
        if (currentStep >= 7) direction = -1;
        if (currentStep <= 0) direction = 1;
      } else if (arpMode == 3) {
        currentStep = random(0, 8);
      }

      int noteToPlay = rootNote + scales[currentScaleIndex][currentStep];

      usbMIDI.sendNoteOn(noteToPlay, noteVelocity, MIDI_CHANNEL);
      activeNote = noteToPlay;
    }

    // gate (staccato)
    unsigned long gateDuration = (speedMillis * gatePercent) / 100;

    if (activeNote != -1 &&
        (now - lastStepTime > gateDuration) &&
        gatePercent < 100) {
      usbMIDI.sendNoteOff(activeNote, 0, MIDI_CHANNEL);
      activeNote = -1;
    }

  } else {
    // silence (hand removed)
    if (activeNote != -1) {
      usbMIDI.sendNoteOff(activeNote, 0, MIDI_CHANNEL);
      activeNote = -1;
    }
  }

  // MIDI buffer flush (Teensy)
  while (usbMIDI.read()) {}

  // 3. DEBUG PRINTING (Every 200 ms)
  // ---------------------------------------------
  if (now - lastDebugPrint > 200) {
    lastDebugPrint = now;

    Serial.print("Dist: ");
    if (cm == 999) Serial.print("---");
    else           Serial.print(cm);

    Serial.print("cm | Speed: ");
    Serial.print(speedMillis);

    Serial.print(" | Knob: ");
    Serial.print(knobValue);

    if (shiftModeActive) {
      Serial.print(" [SHIFT PITCH] Root: ");
      Serial.print(rootNote);
    } else {
      Serial.print(" [GATE] Gate%: ");
      Serial.print(gatePercent);
    }
    Serial.print(" | Flex(raw): ");
    Serial.print(rawFlex);

    Serial.print(" | Bend: ");
    Serial.print(bendValue);

    Serial.print(" | ArpMode: ");
    Serial.print(arpMode);

    Serial.print(" | Button(raw/stable): ");
    Serial.print(rawButton);
    Serial.print("/");
    Serial.print(stableButtonState);
    Serial.print("\n");
  }

  delay(50);
}