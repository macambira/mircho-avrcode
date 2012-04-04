using System;

namespace Master
{
	public class SnapPacket
	{
		public ByteString	Data;
		public uint			Source;
		public uint			Destination;
		public ushort		Crc;

		public SnapPacket()
		{
		}

		public SnapPacket(uint dest, ByteString data)
		{
			Destination = dest;
			Data = data;
		}

		public SnapPacket(uint dest, string text)
		{
			Destination = dest;
			Data = new ByteString(text);
		}
	}
}
