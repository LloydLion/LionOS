#include "LionOSConnect.h"

void initialize()
{
  Serial.println("start");

  OS::addProcess(process1, 3000, "process1");
  //OS::addProcess(process2, 2500, "process2");
}

void process1()
{
  /*****/ ProcessSignature();

  Serial.println("+");
  Delay(500);
  Serial.println("===");
}

void process2()
{
  /*****/ ProcessSignature();


}