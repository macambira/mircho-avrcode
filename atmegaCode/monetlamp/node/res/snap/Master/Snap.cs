using System;
using System.Diagnostics; 
using System.IO.Ports;

namespace Master
{
	public class Snap
	{
		public const uint SnapNodeAddress = 1;

		private SerialPort serialPort;
		private ByteString receiveBuffer = new ByteString();

		private const byte SNAP_PREAMBLE1	= (byte)'!';	// Preamble byte 1
		private const byte SNAP_PREAMBLE2	= (byte)'#';	// Preamble byte 2
		private const byte SNAP_SYNC		= (byte)'T';	// Sync byte (start of SNAP packet)

		private const byte SNAP_DAB		= 0xC0;				// Number of Destination Address Bytes -> 3
		private const byte SNAP_SAB		= 0x30;				// Number of Source Address Bytes -> 3
		private const byte SNAP_PFB		= 0x00;				// Protocol specific Flag Bytes -> 0, unused
		private const byte SNAP_ACK_REQUEST = 0x01;			// Request ACK
		private const byte SNAP_ACK		= 0x02;				// Send ACK
		private const byte SNAP_NAK		= 0x03;				// Send NAK
		private const byte SNAP_CMD		= 0x00;				// Command mode bit -> 0
		private const byte SNAP_EDM		= 0x00;				// Error detection method -> none

		private const byte SNAP_NDB_MASK	= 0x0F;			// Mask bits: Number of data bytes
		private const byte SNAP_DAB_MASK	= 0xC0;			// Mask bits: Number of Destination Address Bytes
		private const byte SNAP_SAB_MASK	= 0x30;			// Mask bits: Number of Source Address Bytes
		private const byte SNAP_ACK_MASK	= 0x03;			// Mask bits: ACK
		private const byte SNAP_CMD_MASK	= 0x80;			// Mask bits: Command mode bit
		private const byte SNAP_EDM_MASK	= 0x70;			// Mask bits: Error detection method
		private const byte SNAP_PFB_MASK	= 0x0C;

		public delegate void SnapPacketReceivedEventHandler(object sender, SnapPacketReceivedEventArgs e);
		public event SnapPacketReceivedEventHandler PacketReceived;

		public Snap()
		{
			// TODO: Use single function to init
			serialPort = new SerialPort("COM1", 9600);
			serialPort.EventFilter = SerialEvents.ReceivedChars;
			serialPort.ReceivedEvent  += new SerialEventHandler(serialPort_OnReceiveEvent);
		}

		public Snap(String port, int baud)
		{
			serialPort = new SerialPort(port, baud);
			serialPort.EventFilter = SerialEvents.ReceivedChars;
			serialPort.ReceivedEvent  += new SerialEventHandler(serialPort_OnReceiveEvent);
		}

		public void Open()
		{
			serialPort.Open();
		}

		public void Close()
		{
			serialPort.Close();
		}

		public void Send(SnapPacket packet)
		{
			byte c;
			int n;
			int i;
			int padding = 0;
			double x;
			byte[] header = new byte[8];


			// Header byte 2
			c = SNAP_DAB | SNAP_SAB | SNAP_PFB | SNAP_ACK_REQUEST;
			header[0] = c;

			// Determine the size of the packet. The protocol allows 0 to 512 bytes of
			// data but not all sizes within that are supported. In the header, if bit 3 = 0,
			// then bits 2..0 indicate the exact size in bytes. If bit 3 = 1, then bits 2..0
			// the size of the data is 2^(bits+3). If the data is greater than 7 bytes then
			// we will pick the next largest size.
			if (packet.Data.Count > 512)
				throw new ApplicationException("Data is too long. 512 byte maximum.");

			// Handle sizes >= 8 differently, see comments above
			if (packet.Data.Count < 8)
				n = packet.Data.Count;
			else
			{
				x = Math.Log(packet.Data.Count, 2);
				if (x > (int)x) 
					n = ((int)x) + 1;
				else
					n = (int)x;
				padding = (int)(Math.Pow(2,n) - packet.Data.Count);
				n = (byte)(n-3) | 0x08;							// Set bit 3 to indicate size is 2^(bits+3) bytes
			}

			// Header byte 1
			c = (byte)(SNAP_CMD | SNAP_EDM | n);
			header[1] = c;

			// Destination address (3 bytes)
			header[2] = (byte)(packet.Destination  >> 16);
			header[3] = (byte)(packet.Destination  >> 8);
			header[4] = (byte)(packet.Destination  >> 0);

			// Source address (3 bytes)
			packet.Source = SnapNodeAddress;
			header[5] = (byte)(packet.Source >> 16);
			header[6] = (byte)(packet.Source >> 8);
			header[7] = (byte)(packet.Source >> 0);

			// Create the packet
			ByteString bytes = new ByteString();

			// Add the preamble and sync bytes
			bytes.Append(SNAP_PREAMBLE1);
			bytes.Append(SNAP_PREAMBLE2);
			bytes.Append(SNAP_SYNC);

			// Add the header and data
			bytes.AddRange(header);
			bytes.AddRange(packet.Data);

			// Pad the data as necessary
			for (i=padding; i>0; i--)
				bytes.Append(0);

			// Calculate and add the CRC (ignore the preamble/sync bytes)
			packet.Crc = Crc.Crc16(bytes.GetBytes(3));
			bytes.Append((byte)(packet.Crc>>8));
			bytes.Append((byte)(packet.Crc>>0));

			// Add a newline for easier debugging
			// The packet is done, these characters should be ignored by receivers
			bytes.Append((byte)'\n');

			// Send the packet
			serialPort.Write(bytes.GetBytes());

		}

		protected void serialPort_OnReceiveEvent(object source, SerialEventArgs e)
		{
			try 
			{
				switch (e.EventType)
				{
					case SerialEvents.ReceivedChars:
						byte[] b = new byte[serialPort.InBufferBytes];
						serialPort.Read(b, 0, b.Length);
						receiveBuffer.AddRange(b);
						findSnapPacket();
						break;

					default:
						Debug.WriteLine("Unknown event: " + e.EventType);
						break;
				}
			}
			catch {}

		}

		protected void findSnapPacket()
		{
			// TODO: Find several packets per call
			byte b;
			int numDataBytes;
			int x;
			SnapPacket packet = new SnapPacket();

			// Search for packet header
			int c = receiveBuffer.IndexOf("!#T");
			if (c >= 0)
			{
				// Drop any data before beginning of packet 
				// (drop the preamble/sync bytes so we don't try
				// and parse this packet again).
				receiveBuffer.RemoveRange(0, c);

				if (receiveBuffer.Count >= 3+2+3+3)
				{

					b = (byte)receiveBuffer[3];
					b = (byte)(b & SNAP_DAB_MASK);
					if (((byte)receiveBuffer[3] & SNAP_DAB_MASK) == SNAP_DAB) 
					{
						if (((byte)receiveBuffer[3] & SNAP_SAB_MASK) == SNAP_SAB) 
						{
							if (((byte)receiveBuffer[3] & SNAP_PFB_MASK) == SNAP_PFB) 
							{
								if (((byte)receiveBuffer[4] & SNAP_CMD_MASK) == SNAP_CMD) 
								{
									if (((byte)receiveBuffer[4] & SNAP_EDM_MASK) == SNAP_EDM) 
									{
										x = ((byte)receiveBuffer[4]) & SNAP_NDB_MASK;
										if (x <= 8)
											numDataBytes = x;
										else
										{
											x = x - 8;
											numDataBytes = 8;
											for ( ; x>0; x-- )
												numDataBytes = numDataBytes * 2;
										}

										packet.Destination = (uint)(byte)receiveBuffer[5] << 16;
										packet.Destination += (uint)(byte)receiveBuffer[6] << 8;
										packet.Destination += (uint)(byte)receiveBuffer[7] << 0;

										packet.Source = (uint)(byte)receiveBuffer[8] << 16;
										packet.Source += (uint)(byte)receiveBuffer[9] << 8;
										packet.Source += (uint)(byte)receiveBuffer[10] << 0;

										if (receiveBuffer.Count >= 3+2+3+3+numDataBytes+2)
										{
											packet.Data = receiveBuffer.GetRange(3+2+3+3, numDataBytes);
											packet.Crc = (ushort)((uint)(byte)receiveBuffer[3+2+3+3+numDataBytes+0] << 8);
											packet.Crc += (ushort)((uint)(byte)receiveBuffer[3+2+3+3+numDataBytes+1] << 0);

											if (packet.Crc == Crc.Crc16(receiveBuffer.GetBytes(3, 2+3+3+numDataBytes)))
											{
												if (packet.Destination == SnapNodeAddress)
												{
													PacketReceived( this, new SnapPacketReceivedEventArgs(packet) ); 
												}
											}

											receiveBuffer.RemoveRange(0, 3+2+3+3+numDataBytes);
										}
									}
									else
										Debug.WriteLine("Only CRC16 is supported");
								}
								else
									Debug.WriteLine("Command mode not supported");
							}
							else
								Debug.WriteLine("Protocol specific flag bytes not supported");
						}
						else
							Debug.WriteLine("Source address not 3 bytes");
					}
					else
						Debug.WriteLine("Destination address not 3 bytes");
				}
			}

		}
	
	}
}
