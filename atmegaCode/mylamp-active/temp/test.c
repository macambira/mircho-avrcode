/*
 * File Name:    test.c
 *
 * Author:        Your Name
 * Creation Date: Tuesday, October 23 2007, 10:52 
 * Last Modified: Wednesday, October 24 2007, 09:53
 * 
 * File Description:
 *
 */

#include <stdio.h>
#include "events.h"

// Create event prototypes:
//HANDLES_EVENT(USB_Connect);
HANDLES_EVENT(Init_Error);

// Functions:
int main (void)
{
   printf("Start.\n");

   RAISE_EVENT(USB_Connect);
   RAISE_EVENT(Init_Error, "TEST", 2);

   printf("End.\n");
   return 0;
}

/*
EVENT_HANDLER(USB_Connect)
{
   printf("Overridden USB Event func.\n");
}
*/

EVENT_HANDLER(Init_Error)
{
   printf("Yaarg %s\n", Reason);
}
