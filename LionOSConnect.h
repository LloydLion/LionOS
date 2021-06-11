  #define LionOS_FrameMode              //Replaces $setup and $loop functions with $initialize function       ## recomended: on
//#define LionOS_NoProramDebug          //Disables all messages via $Debug marcos. Enable in final build      ## recomended: --
  #define LionOS_OSDebug                //Enables OS debug                                                    ## recomended: off
//#define LionOS_SerialSpeed 19200      //Sets auto-serial speed                                              ## recomended: off
//#define LionOS_NoAutoSerial           //Disables auto-serial. !!and $Debug, $Throw and $Warning macoroses   ## recomended: off
//#define LionOS_IntegrationGyverPower  //Enable intergation with GyverPower library(need manual install)     ## recomended: on

#include "LionOS.h"     //Includes LionOS code