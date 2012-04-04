using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics; 
using System.Drawing;
using System.Text;
using System.Windows.Forms;



namespace Master
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{

		private System.Windows.Forms.TextBox txt;
		private System.Windows.Forms.TextBox txtData;
		private System.Windows.Forms.Button btnSend;
		private System.Windows.Forms.Button btnClear;

		private Snap snap;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox txtAddress;
		private System.Windows.Forms.Label label2;

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
			snap = new Snap("COM1", 9600);
			snap.PacketReceived += new Snap.SnapPacketReceivedEventHandler(snap_PacketReceived);
			snap.Open();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.txt = new System.Windows.Forms.TextBox();
			this.txtData = new System.Windows.Forms.TextBox();
			this.btnSend = new System.Windows.Forms.Button();
			this.btnClear = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.txtAddress = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// txt
			// 
			this.txt.Location = new System.Drawing.Point(16, 16);
			this.txt.Multiline = true;
			this.txt.Name = "txt";
			this.txt.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.txt.Size = new System.Drawing.Size(472, 288);
			this.txt.TabIndex = 0;
			this.txt.Text = "";
			// 
			// txtData
			// 
			this.txtData.Location = new System.Drawing.Point(24, 376);
			this.txtData.Name = "txtData";
			this.txtData.Size = new System.Drawing.Size(168, 20);
			this.txtData.TabIndex = 1;
			this.txtData.Text = "A";
			// 
			// btnSend
			// 
			this.btnSend.Location = new System.Drawing.Point(208, 376);
			this.btnSend.Name = "btnSend";
			this.btnSend.Size = new System.Drawing.Size(88, 24);
			this.btnSend.TabIndex = 2;
			this.btnSend.Text = "Send";
			this.btnSend.Click += new System.EventHandler(this.btnSend_Click);
			// 
			// btnClear
			// 
			this.btnClear.Location = new System.Drawing.Point(392, 376);
			this.btnClear.Name = "btnClear";
			this.btnClear.Size = new System.Drawing.Size(88, 24);
			this.btnClear.TabIndex = 4;
			this.btnClear.Text = "Clear";
			this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(24, 360);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(160, 16);
			this.label1.TabIndex = 5;
			this.label1.Text = "Data:";
			// 
			// txtAddress
			// 
			this.txtAddress.Location = new System.Drawing.Point(24, 336);
			this.txtAddress.Name = "txtAddress";
			this.txtAddress.Size = new System.Drawing.Size(168, 20);
			this.txtAddress.TabIndex = 6;
			this.txtAddress.Text = "123";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(24, 320);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(160, 16);
			this.label2.TabIndex = 7;
			this.label2.Text = "Address:";
			// 
			// Form1
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(504, 421);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.txtAddress);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.btnClear);
			this.Controls.Add(this.btnSend);
			this.Controls.Add(this.txtData);
			this.Controls.Add(this.txt);
			this.Name = "Form1";
			this.Text = "Master - SnapTest";
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}


		private void btnSend_Click(object sender, System.EventArgs e)
		{
			snap.Send(new SnapPacket(uint.Parse(txtAddress.Text), txtData.Text));
		}

		private void btnClear_Click(object sender, System.EventArgs e)
		{
			txt.Text ="";
		}

		private void snap_PacketReceived(object sender, SnapPacketReceivedEventArgs e)
		{
			txt.Text += "Snap Packet: " + e.Packet.Source + " -> " + e.Packet.Destination + ": " + e.Packet.Data.ToMixedString() + Environment.NewLine;
			//txt.Text += "CRC ERROR  : " + Source + " -> " + Destination + ": " + Data.ToMixedString() + Environment.NewLine;
		}


	}
}
