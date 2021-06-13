#include "LionOSConnect.h"

void initialize()
{
  Serial.println("start");

  OS::addProcess(process1, 3000, "process1");
}

void process1(ReturnPoint rp)
{
  /*****/ ProcessSignature();

  NavTable(rp)
  {
    NavRecord(1, rp1);
  }

  Serial.println("+");

  Delay(1000, rp1, 1);

  Serial.println("===");
}

void process2()
{
  /*****/ ProcessSignature();


}