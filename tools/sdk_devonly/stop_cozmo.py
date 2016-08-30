import cozmo

'''Dev-only script, stop cozmo
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    coz.drive_wheels(0, 0)
    coz.move_head(0)


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
