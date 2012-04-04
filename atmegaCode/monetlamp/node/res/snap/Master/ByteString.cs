using System;
using System.Collections;
using System.Text;

namespace Master
{
	public class ByteString : ArrayList
	{
		public ByteString()
		{
		}

		public ByteString(String s)
		{
			this.Append(s);
		}

		public ByteString(ByteString bs)
		{
			this.AddRange(bs.GetBytes());
		}

		public void Append(byte b)
		{
			this.Add(b);
		}

		public void Append(String s)
		{
			this.AddRange(Encoding.ASCII.GetBytes(s));
		}

		public byte[] GetBytes()
		{
			return( GetBytes(0, this.Count) );
		}

		public byte[] GetBytes(int index)
		{
			return( GetBytes(index, this.Count-index) );
		}

		public byte[] GetBytes(int index, int count)
		{
			byte[] b = new byte[count];
			this.CopyTo(index, b, 0, count);
			return( b );
		}

		public ByteString Substring(int startIndex, int length)
		{
			ByteString b = new ByteString();
			b.AddRange(this.GetRange(startIndex, length));
			return( b );
		}

		public int IndexOf(string target)
		{
			return ( IndexOf(Encoding.ASCII.GetBytes(target)) );
		}

		public int IndexOf(byte[] target)
		{
			for ( int c=0; c < this.Count - (target.Length-1); c++ )
			{
				if (CompareAt(c, target))
				{
					return( c );
				}
			}
			return( -1 );
		}

		public bool StartsWith(string value)
		{
			return( CompareAt(0, Encoding.ASCII.GetBytes(value)) );
		}

		public bool CompareAt(int index, byte[] target)
		{
			bool match;
			if (this.Count < target.Length)
			{
				match = false;
			}
			else if (target.Length == 0)
			{
				match = false;
			}
			else
			{
				match = true;
				for (int x = 0; x<target.Length; x++)
				{
					if (target[x] != (byte)this[index+x])
					{
						match = false;
						break;
					}
				}
			}
			return( match );
		}

		public override string ToString()
		{
			return Encoding.ASCII.GetString(this.GetBytes());
		}

		public string ToHexString()
		{
			StringBuilder s = new StringBuilder();
			foreach(object o in this)
			{
				if (s.Length>0)
					s.Append(", ");
				s.Append(string.Format("{0:x2}", (byte)o));
			}				
			return "{" + s + "}";
		}

		public string ToMixedString()
		{
			int i;
			StringBuilder s = new StringBuilder();
			Char[] chars = Encoding.ASCII.GetChars(this.GetBytes());
			for (i=0; i<this.Count; i++)
			{
				if (Char.IsLetterOrDigit(chars[i]))
					s.Append(chars[i]);
				else
					s.Append(string.Format("<{0:x2}>", (byte)this[i]));
			}
			return s.ToString();
		}

		public new ByteString GetRange(int index, int count)
		{
			ByteString b = new ByteString();
			b.AddRange(base.GetRange(index, count));
			return b;
		}


/*
		public new byte this[int index]
		{
			get
			{
				return (byte)this[index];
			}
			set
			{
				this[index] = value;
			}
		}
*/
	}
	
}
