#include "EasyNextionLibrary.h"

EasyNex nex(Serial2);

void setup_nextion() {
  nex.begin(9600);
}

void loop_nextion() {
  nex.NextionListen();

  static int time = 0.0;
  static unsigned long last_time = millis();

  unsigned long delta = millis() - last_time;
  last_time = millis();
  time -= delta;

  if (time <= 0.0) {
    if (temp_buf_prev != -1) {
      nex.writeNum("fTIn.val", temp_buf[temp_buf_prev]);
    }

    time = 1000.0;
  }
}

void trigger0() {
  Serial.println("COMPRESSOR ON!");
  switch_relay(RELAY_COMPRESSOR, true);
}

void trigger1() {
  Serial.println("COMPRESSOR OFF!");
  switch_relay(RELAY_COMPRESSOR, false);
}

void trigger2() {
  Serial.println("VENT ON!");
  switch_relay(RELAY_VENT, true);
}

void trigger3() {
  Serial.println("VENT OFF!");
  switch_relay(RELAY_VENT, false);
}

void trigger4() {
  Serial.println("VENT POWER CHANGED!");

  int val = nex.readNumber("sVentSpeed.val");
  if (val == 777777) {
    Serial.println("Error reading power number");
    return;
  }
  
  if (val < 0) {
    val = 0;
  }
  if (val > 255) {
    val = 255;
  }
  val = 100 - val;

  int pwm = (val * 255) / 100;
  analogWrite(PWM_VENT, pwm);
  Serial.print("Vent pwm set to ");
  Serial.println(pwm);
}

void trigger5() {
  Serial.println("FLOW DIRECTION CHANGED!");
}

void trigger6() {
  Serial.println("TIMER START!");
}

void trigger7() {
  Serial.println("TIMER RESET!");
}

