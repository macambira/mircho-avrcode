#ifndef snap2_h__
#define snap2_h__

/************************
 * Address Size Values - in bytes
 ************************/
#define SNAP_ADDRESS_SIZE_0	0
#define SNAP_ADDRESS_SIZE_1	1
#define SNAP_ADDRESS_SIZE_2	2
#define SNAP_ADDRESS_SIZE_3	3


/************************
 * EDM Values
 ************************/
#define SNAP_EDM_NONE		0
#define SNAP_EDM_RETRANSMIT	1
#define SNAP_EDM_CHECKSUM_8	2
#define SNAP_EDM_CRC_8		3
#define SNAP_EDM_CRC_16		4
#define SNAP_EDM_CRC_32		5
#define SNAP_EDM_FEC			6
#define SNAP_EDM_USER		7

/************************
 * NDB Values - in bytes
 ************************/
#define SNAP_NDB_0			0x0000
#define SNAP_NDB_1			0x0001
#define SNAP_NDB_2			0x0002
#define SNAP_NDB_3			0x0003
#define SNAP_NDB_4			0x0004
#define SNAP_NDB_5			0x0005
#define SNAP_NDB_6			0x0006
#define SNAP_NDB_7			0x0007
#define SNAP_NDB_8			0x0008
#define SNAP_NDB_16			0x0009
#define SNAP_NDB_32			0x000a
#define SNAP_NDB_64			0x000b
#define SNAP_NDB_128			0x000c
#define SNAP_NDB_256			0x000d
#define SNAP_NDB_512			0x000e
#define SNAP_NDB_USER		0x000f

/************************
 * ACK Values - in bytes
 ************************/
#define SNAP_REQU_NO_ACK		0x00
#define SNAP_REQU_ACK		0x01
#define SNAP_REPLY_ACK		0x02
#define SNAP_REPLY_NACK		0x03


#define SNAP_ADDRESS_BROADCAST 0

//size, in bytes, of the addresses
#ifndef SNAP_ADDRESS_SIZE
	#define SNAP_ADDRESS_SIZE SNAP_ADDRESS_SIZE_1
#endif

//size, in bytes, of the payload
#ifndef SNAP_NDB_SIZE
	#define SNAP_NDB_SIZE SNAP_NDB_4
#endif

#ifndef SNAP_EDM_TYPE
	#define SNAP_EDM_TYPE SNAP_EDM_CRC_16
#endif

#define SNAP_SYNC			0x54

#endif // snap2_h__
