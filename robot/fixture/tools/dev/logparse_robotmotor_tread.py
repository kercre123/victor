#! python
from logparse_common import LogParser

class LogParserRobotMotorTread(LogParser):

  def __init__(self):
    super().__init__("robotmotor tread", "")

  def __str__(self):
    return super().__str__() + " blah"

