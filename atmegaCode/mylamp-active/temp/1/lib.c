#include <stdio.h>
#include "events.h"

// Prototypes:
void __stub (void);

// Weak symbols:
EVENT_HANDLER(USB_Connect) STUB_ALIAS;
EVENT_HANDLER(Init_Error)  STUB_ALIAS;

void __stub (void)
{

}

