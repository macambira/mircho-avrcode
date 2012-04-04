using System;

namespace Master
{
	public sealed class Crc
	{
		private const ushort CRC16INIT = 0x0000;
		private const ushort CRC16POLY = 0x1021;         //0x1021 = X^16+X^12+X^5+X^0

		public static ushort Crc16(byte[] data)
		{
			uint	crc = CRC16INIT;
			uint	loop_count;
			uint	bit_counter;

			for (loop_count = 0; loop_count != data.Length; loop_count++)
			{
				crc = (ushort)crc;
				crc = ((uint)data[loop_count] << 8) ^ crc;
				for (bit_counter = 0; bit_counter != 8; bit_counter++)
				{
					//check if MSB is one
					if ((crc & 0x8000) == 0x8000)
					{
						crc = (crc << 1) ^ CRC16POLY;
					}
					else
					{
						crc = crc << 1;
					}
				}
			}

			return (ushort)crc;
		}
	}
}
