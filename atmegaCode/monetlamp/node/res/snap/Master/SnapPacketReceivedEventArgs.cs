using System;

namespace Master
{
	public class SnapPacketReceivedEventArgs
	{
		public SnapPacket Packet;
		public SnapPacketReceivedEventArgs(SnapPacket packet)
		{
			Packet = packet;
		}
	}
}
