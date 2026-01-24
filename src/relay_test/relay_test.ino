static const uint8_t RELAY_COMPRESSOR = 43;
static const uint8_t RELAY_VENT = 42;

static const uint8_t RELAY_PINS[] = {RELAY_COMPRESSOR, RELAY_VENT};

static const uint8_t RELAY_OFF = HIGH;
static const uint8_t RELAY_ON = LOW;


void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < sizeof(RELAY_PINS); i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], RELAY_OFF); // default OFF (adjust if your relay board is active-low)
  }
  Serial.println(F("Ready: <pin> on|off (pins 40-43)"));
}

bool isRelayPin(int pin) {
  for (uint8_t i = 0; i < sizeof(RELAY_PINS); i++) if (RELAY_PINS[i] == pin) return true;
  return false;
}

void handleLine(char *s) {
  while (*s == ' ' || *s == '\t') s++;
  if (!*s) return;

  int pin = 0;

  if (strncmp(s, "comp", 4) != 0) {
    pin = RELAY_COMPRESSOR;
  } else if (strncmp(s, "vent", 4) != 0) {
    pin = RELAY_VENT;
  } else {
    pin = atoi(s);
    if (!isRelayPin(pin)) {
      Serial.println(F("ERR: pin (40-43)"));
      return;
    }
  }

  while (*s && *s != ' ' && *s != '\t') s++;
  while (*s == ' ' || *s == '\t') s++;

  bool on;
  if (!strncmp(s, "on", 2)) on = true;
  else if (!strncmp(s, "off", 3)) on = false;
  else {
    Serial.println(F("ERR: cmd (on/off)"));
    return;
  }

  digitalWrite(pin, on ? RELAY_ON : RELAY_OFF); // invert if your relays are active-low
  Serial.print(F("OK "));
  Serial.print(pin);
  Serial.print(' ');
  Serial.println(on ? F("on") : F("off"));
}

void loop() {
  static char buf[32];
  static uint8_t n = 0;

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      buf[n] = 0;
      if (n) handleLine(buf);
      n = 0;
    } else if (n < sizeof(buf) - 1) {
      buf[n++] = c;
    }
  }
}
