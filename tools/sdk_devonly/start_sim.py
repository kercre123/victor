import cozmo

'''Dev-only script, starts engine and connects to a sim robot (for running with a sim robot in Webots with no app)
'''


def start_sim(coz_conn):
    "start sim - starts the engine and connects to a sim robot"
    start_engine_msg = cozmo._clad._clad_to_engine_iface.StartEngine()
    coz_conn.send_msg(start_engine_msg)
    ip_address = "127.0.0.1\0\0\0\0\0\0\0".encode('ascii')
    start_engine_msg = cozmo._clad._clad_to_engine_iface.ConnectToRobot(ipAddress=ip_address, robotID=1, isSimulated=1)
    coz_conn.send_msg(start_engine_msg)

def run(coz_conn):
    start_sim(coz_conn)
    coz = coz_conn.wait_for_robot()
    cozmo.logger.info("Done")

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
