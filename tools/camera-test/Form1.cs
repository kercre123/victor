using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Media;
using System.Net.Sockets;
using System.Net;
using System.Globalization;

namespace BlueCode
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
                serialPort1.Close();

            // Necessary to keep up with 3mbps
            serialPort1.ReadBufferSize = 262144;
            serialPort1.ReceivedBytesThreshold = 256;
            serialPort1.BaudRate = 3000000;

            serialPort1.PortName = COMSelector.Text;
            serialPort1.Open();
            //serialPort1.DataReceived += serialPort1_DataReceived;
        }

        private void COMSelector_Click(object sender, EventArgs e)
        {
            //get list of serial ports
            string[] ports = System.IO.Ports.SerialPort.GetPortNames();
            COMSelector.Items.Clear();
            for (int i = 0; i < ports.Length; i++)
            {
                COMSelector.Items.Add(ports[i]);
            }
        }

        private const int IMGX = 640;
        private const int IMGY = 480;

        private const int IMGFX = IMGX;
        private const int IMGFY = IMGY;

        private const int SCALEX = 2, SCALEY = 2;

        private const bool MULTIFRAME = false;

        // Based on fixed size chunks of 1414 (+1 message type) bytes
        private const int WAITING = -4;
        private const int MAXLEN = 100000;
        private byte[] m_image = new byte[MAXLEN];
        private byte[] m_readyImage = null;
        private int m_imageLen = 0, m_readyLen = 0, m_chunk = 0;

        // Indicates waiting, or header processing (<0), or image reading (0 to imagesize)
        private int m_recvstate = WAITING;
        private int m_skipped = 0;
        private int m_frame = -1;

        private int m_chunkLen = 0, m_chunklets = 0, m_check = 0, m_bigcheck = 0;
        private bool m_lastChunk = false;

        private Image m_bitmap = new Bitmap(IMGX * SCALEX, IMGY * SCALEY);

        private void serialPort1_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            int count = serialPort1.BytesToRead;
            if (count < 1)
                return;

            // Necessary to keep up with 3mbps
            byte[] buf = new byte[count];
            serialPort1.Read(buf, 0, count);

            int idx = 0;

            while (count-- > 0)
            {
                byte c = buf[idx++];

                // Wait for header
                if (m_recvstate < -1)
                {
                    // Test that byte is part of the header
                    if (c == 0xA5)
                    {
                        m_recvstate = -1;
                    }
                    else
                    {
                        m_skipped++;
                        m_recvstate = WAITING;  // Bad byte, back to waiting
                    }

                    // Fill in header
                }
                else if (m_recvstate == -1)
                {
                    m_check = 0;
                    m_chunkLen = (c & 0x7f);
                    m_lastChunk = 0 != (c & 0x80);
                    m_chunklets++;
                    m_recvstate++;
                }
                // Past header, copy image data
                else if (m_recvstate < m_chunkLen)
                {
                    m_check += c;
                    m_image[m_imageLen + m_recvstate] = c;
                    m_recvstate++;
                }
                // At end of chunklet, return to waiting - optionally display what we've got
                if (m_recvstate == m_chunkLen) 
                {
                    m_imageLen += m_chunkLen;
                    m_bigcheck += m_check;

                    if (m_lastChunk)
                    {
                        m_readyImage = m_image;
                        m_readyLen = m_imageLen;

                        int check = 0;
                        for (int i = 0; i < m_imageLen; i++)
                            check += m_image[i];

                        //if ((m_bigcheck & 255) != (check & 255))
                        //    Console.WriteLine("Bad frame sum: K02 says " + (m_bigcheck & 255) + " vs " + (check & 255));

                        m_image = new byte[MAXLEN];

                        this.scanPicBox.BeginInvoke(
                        (MethodInvoker)delegate()
                        {
                            scanPicBox.Refresh();
                        });
                        Console.WriteLine("EOF at " + m_imageLen + " bytes in " + m_chunklets + " chunklets at " + DateTime.Now.Millisecond + "ms");
                        m_bigcheck = m_chunklets = m_imageLen = 0;
                    }

                    if (m_skipped > 0)
                        System.Console.WriteLine("Skipped " + m_skipped + " frame " + m_frame);
                    m_skipped = 0;
                    m_recvstate = WAITING;
                }
            }
        }

        // This is the length of the Reliable Transport header - which we skip completely
        private const int RT_PREFIX = 18;

        // This is the IMAGE_CHUNK message header definition
        private const int HEADERLEN = 14, CHUNKLEN = 1300, PACKETLEN = HEADERLEN + CHUNKLEN;

        private const int MAX_CHUNKS = 16;
        private byte[] m_header = new byte[HEADERLEN];
        
        // Daniel:  This is where the UDP packets are parsed
        private Object m_receiveLock = new Object();
        private bool m_eof;
        private void receive(byte[] buf, int count)
        {
            int idx = 0;

            lock (m_receiveLock)
            {
                m_recvstate = 0;
                idx = RT_PREFIX;          
                count -= RT_PREFIX;

                while (count-- > 0)
                {
                    byte c = buf[idx++];

                    // Process the header bytes
                    if (m_recvstate < HEADERLEN)
                    {
                        m_header[m_recvstate++] = c;

                        // Once header is complete, figure out what to do with the data chunk
                        if (m_recvstate == HEADERLEN)
                        {
                            int id = m_header[0] + (m_header[1] << 8);

                            // Are we done with the old frame?  If so, ship it off
                            if (id != m_frame || m_header[12] != m_chunk+1)
                            {
                                if (!m_eof)
                                {
                                    Console.WriteLine("Dropped frame " + m_frame + " at chunk " + m_chunk);
                                }
                                else if (m_frame != -1)
                                {
                                    m_readyImage = m_image;
                                    m_readyLen = m_imageLen;
                                    this.scanPicBox.BeginInvoke(
                                    (MethodInvoker)delegate()
                                    {
                                        scanPicBox.Refresh();
                                    });
                                    System.Console.WriteLine("Frame: " + m_frame + "  ms: " + DateTime.Now.Millisecond);
                                }

                                m_image = new byte[MAXLEN];
                                m_frame = id;

                                if (m_header[12] != 0) // A new frame must start at chunk 0
                                    m_frame = -1;
                            }

                            int len = m_header[8] + (m_header[9] << 8);
                            int chunks = m_header[11];
                            m_chunk = m_header[12];
                            m_eof = chunks != 255;
                            m_imageLen = m_chunk * CHUNKLEN + len;
                            //Console.WriteLine("ID " + id + " Chunk " + m_chunk + "/" + chunks + " Len " + len);
                        }
                    }
                    // Past header, copy image data
                    else if (m_recvstate < PACKETLEN)
                    {
                        if (m_chunk < 0 || m_chunk >= MAX_CHUNKS)
                        {
                            Console.WriteLine("Illegal chunk " + m_chunk);
                            m_recvstate = WAITING;
                            continue;
                        }
                        m_image[m_chunk * CHUNKLEN + m_recvstate - HEADERLEN] = c;
                        m_recvstate++;
                    }
                    // Past image data, display image and return to waiting
                    if (m_recvstate >= PACKETLEN)
                    {
                        if (m_skipped > 0)
                            System.Console.WriteLine("Skipped " + m_skipped + " frame " + m_frame);
                        m_skipped = 0;
                        m_recvstate = WAITING;
                    }
                }
            }
        }

        private void receiveNoHeader(byte[] buf, int start, int end, bool eof)
        {
            int len = end - start;
            for (int i = 0; i < len; i++)
                m_image[i + m_imageLen] = buf[i + start];

            m_imageLen += len;
            //System.Console.WriteLine(len + "/" + m_imageLen);

            if (eof)
            {
                m_readyImage = m_image;
                m_readyLen = m_imageLen;
                this.scanPicBox.BeginInvoke(
                (MethodInvoker)delegate()
                {
                    scanPicBox.Refresh();
                });
                System.Console.WriteLine("Frame: " + m_frame + "  ms: " + DateTime.Now.Millisecond);

                m_image = new byte[MAXLEN];
                m_imageLen = 0;
            }
        }


        // Turn a fully assembled MINIPEG_GRAY image into a JPEG with header and footer
        private byte[] miniGrayToJpeg(byte[] bin, int len)
        {
            // Pre-baked JPEG header for grayscale
            byte[] header = new byte[] {
	            0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 
	            0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, // 0x19 = QTable
	            0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 
	            0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 
	            0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 
	            0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0, // 0x5E = Height x Width
	            0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 
	            0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 
	            0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 
	            0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 
	            0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 
	            0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 
	            0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 
	            0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 
	            0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 
	            0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 
	            0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 
	            0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 
	            0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 
	            0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 
	            0x00, 0x00, 0x3F, 0x00
            };

            int width = 320, height = 240;
            header[0x5f] = (byte)height;
            header[0x5e] = (byte)(height >> 8);
            header[0x61] = (byte)width;
            header[0x60] = (byte)(width >> 8);

            // Allocate enough space for worst case expansion
            byte[] bout = new byte[len * 2 + header.Length];

            int off = header.Length;
            Array.Copy(header, bout, off);

            // Fetch quality
            int qual = bin[0];

            // Add byte stuffing - one 0 after each 0xff
            for (int i = 1; i < len; i++)
            {
                bout[off++] = bin[i];
                if (bin[i] == 0xff)
                {
                    bout[off++] = 0;
                    //Console.WriteLine("Stuffed at " + i);
                }
            }

            bout[off++] = 0xFF;
            bout[off++] = 0xD9;

            return bout;
        }

        private void scanPicBox_Paint(object sender, PaintEventArgs e)
        {
            if (m_readyImage == null)
                return;

            try
            {
                byte[] image = m_readyImage;
                int len = m_readyLen;
                
                byte[] image2 = miniGrayToJpeg(image, len);
                MemoryStream ms = new MemoryStream(image2, 0, image2.Length);
                scanPicBox.Image = Image.FromStream(ms);
                //m_udp.Send(new byte[] { 18, 0, 0, 0, 0, m_header[4], m_header[5], m_header[6], m_header[7] }, 9);

                if (m_snapshot)
                {
                    String name = "cam-" + DateTime.UtcNow.ToString("HHmmssff") + ".jpg";
                    Stream str = File.Create(name);
                    str.Write(image2, 0, image2.Length);
                    str.Close();
                    m_snapshot = false;
                }
            }
            catch (Exception e2)
            {
                Console.WriteLine("Couldn't decode JPEG due to:\n" + e2);
            }

            m_readyImage = null;
        }

        private bool m_snapshot;
        private void butSnapshot_Click(object sender, EventArgs e)
        {
            m_snapshot = true;
        }

        private void btnCSV_Click(object sender, EventArgs e)
        {
            txtCSV.Visible = true;
            txtCSV.Text = "Paste data here";
            Application.DoEvents();
            txtCSV.Select();
            //txtCSV.SelectAll();
        }

        private void txtCSV_TextChanged(object sender, EventArgs e)
        {
            try
            {
                String[] lines = txtCSV.Text.Split('\r');
                byte[] image = new byte[IMGFX * IMGFY];
                if (lines.Length < 60)
                    return;
                for (int y = 0; y < 60; y++)
                {
                    String[] cols = lines[y].Split(',');
                    for (int x = 0; x < 80; x++)
                        image[(IMGFX-1 - x) + IMGFX * (IMGFY-1 - y)] = Byte.Parse(cols[1 + x]);
                }

                m_readyImage = image;
                this.scanPicBox.BeginInvoke(
                (MethodInvoker)delegate()
                {
                    scanPicBox.Refresh();
                });
                txtCSV.Visible = false;
            }
            catch (Exception x)
            { }
        }

        UdpClient m_udp = null;
        IPEndPoint m_ep = null;
        byte[] m_udpBuffer = new byte[4096];

        static int packetcount = 0;
        private void btnTCP_Click(object sender, EventArgs e)
        {
            try
            {
                m_udp = new UdpClient("172.31.1.1", 5551);
                m_ep = new IPEndPoint(IPAddress.Any, 5550);
                m_udp.Send(new byte[] { (byte)'0' }, 1);
                //m_udp.Send(new byte[] { 18,  0, 0, 0, 0,  0, 0, 0, 0 }, 9);
                m_udp.BeginReceive(new AsyncCallback(UdpReceive), null);
                packetcount = 0;
            }
            catch (Exception x)
            {
                MessageBox.Show("UDP connection failed:\n" + x.Message);
            }
        }

        int expect = 0;
        static int LINELEN = 96;
        static byte[][] bufs = new byte[5000][];
        private void UdpReceive(IAsyncResult ar)
        {
            try
            {
                byte[] buf = m_udp.EndReceive(ar, ref m_ep);
                packetcount++;
                bufs[packetcount] = buf;
                //Console.WriteLine(buf.Length);

                // Parse reliable transport header, copying any image data we find
                // See "robot reliable transport in brief.md"
                int i = 14;     // Skip RT header (14 bytes)

                // While there could be another chunk for us to read...
                while (i + 8 < buf.Length)
                {
                    int len = buf[i + 1] + buf[i + 2] * 256;
                    i += 3;
                    int tag = buf[i];
                    if (tag == 130)
                    {
                        i += 15;        // Skip CLAD type + image chunk header
                        len -= 15;
                        int chunkCount = buf[i - 4];
                        int chunkId = buf[i - 3];
                        bool eof = chunkCount != 0;
                        //Console.WriteLine(chunkCount + " " + chunkId + " " + len);
                        receiveNoHeader(buf, i, i + len, eof);
                    }
                    i += len;
                } 

                //receiveNoHeader(buf);
            }
            catch (Exception x)
            {
                Console.WriteLine("Oops");
            }
            // Prepare for the next receive
            m_udp.BeginReceive(new AsyncCallback(UdpReceive), null);

            /*
            this.scanPicBox.BeginInvoke(
            (MethodInvoker)delegate()
            {
                scanPicBox.Refresh();
            });
            */
        }
    }
}
