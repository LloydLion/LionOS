#include "LionOSConnect.h"

void initialize()
{
  Serial.println("start");

  OS::addHandlerProcess(handler, checker, "process2", 3000);
  //OS::addProcess(handler, 3000, "process2");
}

void process1(ReturnPoint rp)
{
  // /*****/ ProcessSignature();
  // /*****/ Process_DisallowManualInvoke();
  // /*****/ NavTable(rp) { NavRecord(1, rp1); NTE; }

  // Var(int, i, 1);

  // Serial.print(i);

  // Delay(1000, rp1, 1);
  
  // Serial.print("-");
  // Serial.println(++i);
}

void handler(ReturnPoint rp)
{
  /*****/ ProcessSignature();

  Serial.println("asd");
}

ReturnPoint checker()
{
  bool av = Serial.available();
  if(av) Serial.readString();
  return av ? 0 : -1;
}