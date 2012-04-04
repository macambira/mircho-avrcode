#ifndef EVENTS_H
#define EVENTS_H

	// Event Defines:
	#define RAISE_EVENT(e, ...)    Event_ ## e (__VA_ARGS__)
	#define EVENT_HANDLER(e)       void e
	#define HANDLES_EVENT(e)       void e e ## _M
	#define ALIAS_STUB(e)          void e __attribute__((weak, alias("__stub")))

	// Event Modifiers:
	#define DEPRECATED             __attribute__((deprecated))
 	#define NO_MODIFIER

	// Event Prototypes:
	#define USB_Connect            Event_USB_Connect (void)
	#define USB_Connect_M          DEPRECATED

	#define Init_Error             Event_Init_Error  (char* Reason, int Num)
	#define Init_Error_M           NO_MODIFIER

	// Events File Prototypes:
	#ifdef INCLUDE_FROM_EVENTS_H
		static void __stub (void);

		ALIAS_STUB(USB_Connect);
		ALIAS_STUB(Init_Error);
	#endif

#endif
