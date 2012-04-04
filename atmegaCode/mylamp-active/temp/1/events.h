// Defines:
#define EVENT_HANDLER(x)    void x ## _SR
#define HANDLES_EVENT(x)    void x ## _HND
#define RAISE_EVENT(x, ...) Event_ ## x (__VA_ARGS__)
#define STUB_ALIAS          __attribute__((weak, alias("__stub")))
#define DEPRECATED          __attribute__((deprecated))

// Event Defines:
#define USB_Connect_HND USB_Connect_SR DEPRECATED
#define Init_Error_HND  Init_Error_SR

#define USB_Connect_SR  Event_USB_Connect (void)
#define Init_Error_SR   Event_Init_Error  (char* Reason)

