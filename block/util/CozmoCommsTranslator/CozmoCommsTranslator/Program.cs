/*********************************************************************************
 * CozmoCommsTranslator
 * 
 * A utility for bidirectionally moving raw bytes between a serial COM port and a TCP socket.
 * Made to be used with a physical robot that uses in a serial port (in lieu of a BTLE radio)
 * to communicate with a basestation. 
 * 
 * Setup:
 * - Connect robot to serial port on Windows PC.
 * - Start Webots with a world containing a robot_advertisement_controller on Mac. 
 * - Start basestation separately or as a controller in the Webots world (on the Mac). 
 * - Start this utility with proper arguments: 
 *      CozmoCommsTranslator robotID serialCOMPort advertisementServiceIP advertisementServicePort 
 *  
 *     robotID:                            Single byte number representing the ID of the robot
 *     serialCOMPort:                  The X in COMX at which the physical robot is attached
 *     advertisementServiceIP:     The IP address of the advertisement service (which is wherever Webots is running)
 *     advertisementServicePort:  The registration port of the advertisement service
 * 
 *********************************************************************************/

#define USE_SERIAL   // For testing: comment out to ignore COM port or lack thereof

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.IO.Ports;




namespace CozmoCommsTranslator
{
    class Program
    {
        // Duplicating message from cozmoConfig.h
        public class RobotAdvertisementRegistration {
            static int SIZEOF_ROBOTADDR = 18;
            public static int TOTAL_NUM_BYTES = 2 + SIZEOF_ROBOTADDR + 1 + 1;

            public RobotAdvertisementRegistration(byte robotID, IPAddress robotAddr, ushort port, bool enableAdvertisement) {
                robotAddr_ = new byte[SIZEOF_ROBOTADDR];
                Buffer.BlockCopy(Encoding.ASCII.GetBytes(robotAddr.ToString()), 0, robotAddr_, 0, robotAddr.ToString().Length);
                robotAddr_[robotAddr.ToString().Length] = 0; // Null terminate string
                robotID_ = robotID;
                port_ = port;
                EnableAdvertisement(enableAdvertisement);

                Console.WriteLine("Advertising available robot at " + robotAddr.ToString() + " on port " + port);
                Console.WriteLine(ByteArrayToString(Encoding.ASCII.GetBytes(robotAddr.ToString())));
            }

            public byte[] GetBytes()
            {
                byte[] res = new byte[TOTAL_NUM_BYTES];

                int offset = 0;
                Buffer.BlockCopy(BitConverter.GetBytes(port_), 0, res, 0, sizeof(ushort));
                offset += sizeof(ushort);
                Buffer.BlockCopy(robotAddr_, 0, res, offset, SIZEOF_ROBOTADDR);
                offset += SIZEOF_ROBOTADDR;
                res[offset] = robotID_;
                offset++;
                res[offset] = enableAdvertisement_;

                return res;
            }

            public void EnableAdvertisement(bool enable) {
                if (enable)
                    enableAdvertisement_ = 1;
                else
                    enableAdvertisement_ = 0;
            }
            
            public ushort port_;                // Port that robot is accepting connections on
            public byte[] robotAddr_;        // IP address as null terminated string            
            public byte robotID_;
            public byte enableAdvertisement_;  // 1 when robot wants to advertise, 0 otherwise.

        };


        // Port on which to listen for connecting basestation
        public const Int32 listenPort = 5551;
        
        static void Main(string[] args)
        {
            try
            {
                // Get args
                if (args.Length < 4)
                {
                    Console.WriteLine("USAGE: CozmoCommsTranslater robotID serialComPort advertisementServiceIP advertisementServicePort\n");
                    Console.WriteLine("             robotID:                            Single byte number representing the ID of the robot");
                    Console.WriteLine("             serialCOMPort:                  The X in COMX at which the physical robot is attached");
                    Console.WriteLine("             advertisementServiceIP:     The IP address of the advertisement service (which is wherever Webots is running)");
                    Console.WriteLine("             advertisementServicePort:  The registration port of the advertisement service\n");
                    return;
                }

                // Get robot ID
                byte robotID = 0; 
                try 
                {
                    robotID = Convert.ToByte(args[0]);
                } catch (Exception e)
                {
                    Console.WriteLine("Invalid robot ID " + args[0]);
                    return;
                }

                // Get COM port
                string robotComPort = "";
                try
                {
                    int n = int.Parse(args[1]);                    
                    robotComPort = "COM" + args[1];
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid com port " + args[1]);
                    return;
                }


                // Get robot advertisement server IP and port
                IPAddress advertisementServerIP = null;
                try
                {
                    advertisementServerIP = IPAddress.Parse(args[2]);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid IP address for robot advertisement service.");
                    return;
                }

                ushort advertisementServerPort = 0;
                try
                {
                    advertisementServerPort = Convert.ToUInt16(args[3]);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid port for robot advertisement service");
                    return;
                }


                // Setup serial comms
                SerialPort serialPort = new SerialPort();
                serialPort.PortName = robotComPort;
                serialPort.BaudRate = 1000000;
                serialPort.DataBits = 8;
                serialPort.StopBits = StopBits.One;
                serialPort.Handshake = Handshake.None;
                serialPort.Parity = Parity.None;

#if (USE_SERIAL)
                try
                {
                    serialPort.Open();
                }
                catch (Exception e)
                {
                    Console.WriteLine("Serial port failed to open");
                    return;
                }
#endif
                // Buffer for shuffling data
                Byte[] bytes = new Byte[4096];


                // Setup TCP server for basestation comms
                string localIP = LocalIPAddress();
                Console.WriteLine("Local IP: " + localIP);
                IPAddress localAddr = IPAddress.Parse(localIP);
                TcpListener server = new TcpListener(localAddr, listenPort);
                server.Start();  // Start listening for clients


                // Register with advertisement service
                Console.WriteLine("Registering with advertisement service at " + advertisementServerIP.ToString()
                                    + " port " + advertisementServerPort.ToString());
                UdpClient advertisementClient = new UdpClient();
                advertisementClient.Connect(advertisementServerIP, advertisementServerPort);

                RobotAdvertisementRegistration regMsg = new RobotAdvertisementRegistration(robotID, ((IPEndPoint)server.LocalEndpoint).Address, listenPort, true);
                advertisementClient.Send(regMsg.GetBytes(), RobotAdvertisementRegistration.TOTAL_NUM_BYTES);
                



                // TODO: Shuffle data in one direction in one thread and in the other direction in another thread?

                TcpClient bsClient = null;
                NetworkStream bsStream = null;
                int bytesRead = 0;
                while (true)
                {
                    // Wait for basestation connection
                    if (bsClient == null)
                    {
                        Console.WriteLine("Waiting for basestation client...");
                        bsClient = server.AcceptTcpClient(); // blocking
                        bsStream = bsClient.GetStream();
                        Console.WriteLine("*** Basestation CONNECTED ***");

                        // Deregister from advertisement service
                        regMsg.EnableAdvertisement(false);
                        advertisementClient.Send(regMsg.GetBytes(), RobotAdvertisementRegistration.TOTAL_NUM_BYTES);

#if (USE_SERIAL)
                        // Purge serial line since whatever is in there was sent before the connection/
                        while (serialPort.BytesToRead > 0)
                        {
                            bytesRead = serialPort.Read(bytes, 0, Math.Min(serialPort.BytesToRead, bytes.Length));
                            bsStream.Write(bytes, 0, bytesRead);
                        }
#endif 
                    }


                    try
                    {
                        // Send data from basestation to robot
                        while (bsClient.Available > 0) {
                            bytesRead = bsStream.Read(bytes, 0, Math.Min(bsClient.Available, bytes.Length));
                            Console.WriteLine("BS-READ " + bytesRead + " bytes");

#if (USE_SERIAL)
                            if (bytesRead > 0) {
                                serialPort.Write(bytes, 0, bytesRead);
                            }
#endif
                        }


                        // Send data from robot to basestation

#if (USE_SERIAL)
                        // TODO:  Check for timeout?
                        while (serialPort.BytesToRead > 0)
                        {
                            bytesRead = serialPort.Read(bytes, 0, Math.Min(serialPort.BytesToRead, bytes.Length));
                            Console.WriteLine("ROBOT-READ " + bytesRead + " bytes ");
                            bsStream.Write(bytes, 0, bytesRead);
                        }
#endif
                    }
                    catch (Exception e)
                    {
                        // Client has probably disconnected
                        Console.WriteLine("*** Basestation DISCONNECTED ***: " + e.Message);
                        bsStream.Close();
                        bsClient.Close();

                        bsStream = null;
                        bsClient = null;

                        // Register with advertisement service
                        regMsg.EnableAdvertisement(true);
                        advertisementClient.Send(regMsg.GetBytes(), RobotAdvertisementRegistration.TOTAL_NUM_BYTES);
                    }


                }

                bsStream.Close();
                bsClient.Close();
                server.Stop();
                advertisementClient.Close();
            }
            catch (Exception e)
            {
                Console.WriteLine("ERROR: " + e.Message);
            }
        }
        /*
        public static string ByteArrayToString(byte[] ba)
        {
            StringBuilder hex = new StringBuilder(ba.Length * 2);
            foreach (byte b in ba)
            hex.AppendFormat("{0:x2}", b);
            return hex.ToString();
        }
        */
        public static string ByteArrayToString(byte[] ba)
        {
            string hex = BitConverter.ToString(ba);
            return hex.Replace("-","");
        }

        public static string LocalIPAddress()
        {
            IPHostEntry host;
            string localIP = "";
            host = Dns.GetHostEntry(Dns.GetHostName());
            foreach (IPAddress ip in host.AddressList)
            {
                if (ip.AddressFamily == AddressFamily.InterNetwork)
                {
                    localIP = ip.ToString();
                    break;
                }
            }
            return localIP;
        }

    }
}
