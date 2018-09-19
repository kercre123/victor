#! python

class LogParser:
  name=""
  meta=""
  
  def __init__(self, name, meta=""):
    self.name = name
    self.meta = meta

  def reset(self):
    print("reset")
  
  def start(self):
    print("start")
  
  def stop(self):
    print("stop")
  
  def result(self, result):
    pass
  
  def line(self, line):
    raise NotImplementedError("Inheriting class must implement this method")
  
  def __str__(self):
    return self.name + " " + self.meta

