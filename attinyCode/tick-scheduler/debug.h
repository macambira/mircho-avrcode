#ifndef DEBUG_MODULE
#define DEBUG_MODULE


#ifdef __cplusplus
extern "C" {
#endif

uint8_t DBG_init( uint8_t intervalms );
uint8_t DBG_queueByte( uint8_t byte );

#ifdef __cplusplus
}
#endif

#endif