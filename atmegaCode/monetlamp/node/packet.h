#ifndef packet_h__
#define packet_h__

struct _packet_status 
{
	enum
	{
		IDLE,
		WAITING_FOR_TRANSMIT,
		TRANSMITTING,
		WAITING_FOR_RECEIVE,
		RECEIVING
	} state;
	enum
	{
		NO_ERROR,
		RX_CANCEL,
		TX_CANCEL
	} errorStatus;
} packetStatus;

typedef struct _packetPayload
{
	uint8_t recvAddress;
	uint8_t sendAddress;
	uint8_t commandByte;
	uint8_t dataByte1;
	uint8_t dataByte2;
	uint8_t crc8;
} tPacketPayload;

typedef union _packet
{
	tPacketPayload pdata;
	uint8_t dataBytes[ sizeof(tPacketPayload) ];
} tPacket;

tPacket thePacket;

void sendPacket();
void readPacket();
uint8_t isPacketValid();
uint8_t isPacketMulticast();
 

#endif // snap2_h__