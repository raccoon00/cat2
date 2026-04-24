#include <Adafruit_MAX31865.h>
#include <Wire.h>
#include <GyverDS18.h>




static const int RELAY_COMPRESSOR = 43;
static const int RELAY_VENT = 42;
static const int PWM_VENT = 44;
static const int ONE_WIRE_BUS = 46;

static const int SERVO = 45;

static const int RELAY_OFF = HIGH;
static const int RELAY_ON = LOW;


static const int TEMP_BUF_SIZE = 32;
double temp_buf[TEMP_BUF_SIZE];
int temp_buf_size = 0;
int temp_buf_cur = 0;
int temp_buf_prev = -1;

GyverDS18Single temp_ds(A10);


void fetch_temp() { 
  if (temp_buf_prev != -1) {
    Serial.print("Temp: ");
    Serial.println(temp_buf[temp_buf_prev]);
  }
}


void setup_hardware() {
  // I2C
  Wire.begin();

  // 1-WIRE
  temp_ds.setResolution(12);

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

  if (temp_ds.tick() == 0) {
    double data = temp_ds.getTemp();
    temp_buf[temp_buf_cur] = data;
    temp_buf_prev = temp_buf_cur;
    temp_buf_cur = (temp_buf_cur + 1) % TEMP_BUF_SIZE;
    if (temp_buf_size < TEMP_BUF_SIZE) temp_buf_size++;
  }
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
  Serial.println(" - scan");
  Serial.println(" - temp");
  Serial.println("");
}


void scan_i2c() {
  int nDevices = 0;

  Serial.println("Scanning...");

  for (byte address = 1; address < 127; ++address) {
    // The i2c_scanner uses the return value of
    // the Wire.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");

      ++nDevices;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
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
  } else if (strncmp(s, "scan", 4) == 0) {
    scan_i2c();
  } else if (strncmp(s, "temp", 4) == 0) {
    fetch_temp();
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


