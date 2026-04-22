#include "EasyNextionLibrary.h"

EasyNex nex(Serial1);

void setup_nextion() {
  nex.begin(9600);
}

void loop_nextion() {
  nex.NextionListen();
}

