#include <Adafruit_MAX31865.h>


static const int RELAY_COMPRESSOR = 43;
static const int RELAY_VENT = 42;
static const int PWM_VENT = 44;

static const int SERVO = 45;

static const int RELAY_OFF = HIGH;
static const int RELAY_ON = LOW;


Adafruit_MAX31865 thermo = Adafruit_MAX31865(22, 23, 19, 18);
#define RREF      430.0
#define RNOMINAL  100.0


void setup_hardware() {
  // Relays
  pinMode(RELAY_VENT, OUTPUT);
  pinMode(RELAY_COMPRESSOR, OUTPUT);
  
  digitalWrite(RELAY_VENT, RELAY_OFF);
  digitalWrite(RELAY_COMPRESSOR, RELAY_OFF);

  // PWM
  pinMode(SERVO, OUTPUT);
  pinMode(PWM_VENT, OUTPUT);

  // Help output
  print_help();
}


void loop_hardware() {
  serial_control_fetch();

  air_mass_fetch();
}


void max_loop() {
  static const int max_every = 2000;
  static unsigned long last_time = 0;

  unsigned long cur_time = millis();
  if (cur_time - last_time < max_every) return;
  last_time = cur_time;

  uint16_t rtd = thermo.readRTD();

  Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  Serial.print("Ratio = "); Serial.println(ratio,8);
  Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Temperature = "); Serial.println(thermo.temperature(RNOMINAL, RREF));

  // Check and print any faults
  uint8_t fault = thermo.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold"); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold"); 
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias"); 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage"); 
    }
    thermo.clearFault();
  }
  Serial.println();
}


void serial_control_fetch() {
  static const size_t SERIAL_CONTROL_BUF_SIZE = 32;
  static char buf[SERIAL_CONTROL_BUF_SIZE];
  static uint8_t n = 0;

  while (Serial.available()) {
    char c = (char)Serial.read();

    if (n >= SERIAL_CONTROL_BUF_SIZE) {
      n = 0;
      Serial.println("WARNING: serial control buffer overflow!");
    }

    if (c == '\r' || c == '\n') {
      buf[n] = 0;
      if (n) handleLine(buf);
      n = 0;
    } else if (n < SERIAL_CONTROL_BUF_SIZE) {
      buf[n] = c;
      n += 1;
    }
  }
}


void print_help() {
  Serial.println("Available commands:");
  Serial.println(" - vent on/off/%");
  Serial.println(" - servo %");
  Serial.println(" - comp on/off");
  Serial.println("");
}


char *skip_init(char *s) {
  while (*s && *s != ' ' && *s != '\t') s++;
  while (*s == ' ' || *s == '\t') s++;
  return s;
}


void handleLine(char *s) {
  while (*s == ' ' || *s == '\t') s++;
  if (!*s) return;

  if (strncmp(s, "help", 4) == 0 || strncmp(s, "?", 1) == 0) {
    print_help();
  } else if (strncmp(s, "comp", 4) == 0) {
    handle_comp(skip_init(s));
  } else if (strncmp(s, "vent", 4) == 0) {
    handle_vent(skip_init(s));
  } else if (strncmp(s, "servo", 5) == 0) {
    handle_servo(skip_init(s));
  } else {
    Serial.println(F("ERR: no such device"));
  }
}


bool handle_relay(char *s, uint8_t pin) {
  bool on;
  if (strncmp(s, "on", 2) == 0) on = true;
  else if (strncmp(s, "off", 3) == 0) on = false;
  else return false;

  digitalWrite(pin, on ? RELAY_ON : RELAY_OFF);

  Serial.print(F("Vent Relay "));
  Serial.println(on ? F("on") : F("off"));
  return true;
}


void handle_comp(char *s) {
  bool handled = handle_relay(s, RELAY_VENT);
  if (handled) return;
}


void handle_vent(char *s) {
  bool handled = handle_relay(s, RELAY_VENT);
  if (handled) {
    Serial.println(F("Vent Relay switched"));
    return;
  }

  int pct = atoi(s);
  Serial.print(F("Vent value: "));
  Serial.println(pct);

  int pwm = (pct * 255) / 100;
  analogWrite(PWM_VENT, pwm);
}


void handle_servo(char *s) {
  int pct = atoi(s);
  int pwm = (pct * 255) / 100;
  Serial.print(F("Servo angle: "));
  Serial.println(pwm);

  analogWrite(SERVO, pwm);
}


void air_mass_fetch() {
  static unsigned long last_update = 0;
  static int threshold_up = 700;
  static int threshold_down = 300;
  static int state = 0;
  static int count = 0;

  int data = analogRead(A0);
  if (state == 0 && data > threshold_up) {
    state = 1;
    count++;
  } else if (state == 1 && data < threshold_down) {
    state = 0;
  }

  unsigned long time = millis();
  if ((time - last_update) > 1000) {
    last_update = time;
    // Serial.println(count);
    // Serial.print("Analog A0: ");
    // Serial.println(data);
    count = 0;
  }
}


