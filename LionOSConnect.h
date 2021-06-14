  #define LionOS_FrameMode                  //Replaces $setup and $loop functions with $initialize function       ## recomended: on
//#define LionOS_NoProramDebug              //Disables all messages via $Debug marcos. Enable in final build      ## recomended: --
  #define LionOS_OSDebug                    //Enables OS debug                                                    ## recomended: off
//#define LionOS_SerialSpeed 38400          //Sets auto-serial speed                                              ## recomended: off
//#define LionOS_NoAutoSerial               //Disables auto-serial. !!and $Debug, $Throw and $Warning macoroses   ## recomended: off
//#define LionOS_IntegrationGyverPower      //Enable intergation with GyverPower library (need manual install)    ## recomended: on (ATmega 328p/168/2560) off (Other)
//#define LionOS_IGP_ManualWDCalibrate 228  //Calibrates Wathdog with number in GyverPower (see lib tutorials)    ## recomended: set

#include "LionOS.h"     //Includes LionOS code