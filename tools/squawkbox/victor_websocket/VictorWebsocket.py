import websocket
import json
import threading
import logging
from statistics import mode, StatisticsError

class VictorWebsocket(object):
  def __init__(self, robot_ip):
    self.logger = self.logging()

    websocket.enableTrace(True)
    self.robot_ip = robot_ip
    #Generic subscribe message, to request specific data type from Vic websocket
    self.subscribeData = {"type": "subscribe", "module": None}
    
    self.intent_port = 8888
    self.micdata_port = 8889

    self.num_audio_directions = 12
    
    #TODO: Make this a blocking action, until a websocket connection is esablished
    self.mic_socket_established = False
    self.mic_stream_flag = False
    
    self.cloud_intent_socket_established = False
    self.cloud_intent_stream_flag = False

    self.mic_data_raw_results = {"dominant": [], "confidence": [0]*12}
    self.cloud_intent_data_raw_results = []

    self.init_micdata_socket()
    self.init_cloud_intent_socket()

  def init_micdata_socket(self):
    mic_websocket_url = "ws://{0}:{1}/socket".format(self.robot_ip, self.micdata_port)
    
    self.ws_micdata = websocket.WebSocketApp(mic_websocket_url,
                                    on_message = self.on_mic_message,
                                    on_error = self.on_error,
                                    on_close = self.on_close)
    
    self.ws_micdata.on_open = self.on_open_mic
    self.ws_micdata.keep_running = True 

    #Seperate background thread, to handle incoming messages
    self.wst_micdata = threading.Thread(target=self.ws_micdata.run_forever)
    self.wst_micdata.daemon = True 
    self.wst_micdata.start()

  def close_mic_socket(self):
    self.ws_micdata.keep_running = False
    return

  def on_open_mic(self, ws):
    self.subscribeData["module"] = "micdata"
    subscribe_message = json.dumps(self.subscribeData)
    self.logger.info("Now waiting for a reply")
    ws.send(subscribe_message)
    self.mic_socket_established = True

  def on_mic_message(self, ws, message):
    if self.mic_stream_flag == True:
      self.process_micdata_stream(message)
  
  def process_micdata_stream(self, message):
    json_data = json.loads(message)
    dominant = json_data["data"]["dominant"]
    confidence = json_data["data"]["confidence"]

    #There should only be 12 audio directions
    if dominant >= self.num_audio_directions: 
        return
    else:
        self.mic_data_raw_results['dominant'].append(dominant)
        self.mic_data_raw_results['confidence'][dominant] += confidence
  
  def compile_mic_results(self):
    #Find mode of the 12 audio directions

    dominant_list = self.mic_data_raw_results['dominant']
    confidence_list = self.mic_data_raw_results['confidence']
    
    try:
      final_direction = mode(dominant_list)
    except StatisticsError:
      if not dominant_list: 
        self.logger.error("mic_data dominant_list was empty")
        return None
      else:
        #TODO: Should give all directions that equally occured
        #Currently givesthe equally occured direction, with highest 
        #int value i.e [0,0,1,1] = 1
        final_direction = max(dominant_list, key=dominant_list.count)
    
    #Calculate the average confidence for that direction
    final_direction_confidence = confidence_list[final_direction]
    final_direction_samples = len(dominant_list)
    final_confidence_avg = final_direction_confidence / final_direction_samples
    
    #reset raw results
    self.mic_data_raw_results["dominant"].clear()
    self.mic_data_raw_results["confidence"] = [0] * 12 
    
    results_dict = {"final_dominant": final_direction,
                    "final_confidence": final_confidence_avg}

    return results_dict

  def init_cloud_intent_socket(self):
    cloud_intent_websocket_url = "ws://{0}:{1}/socket".format(self.robot_ip, self.intent_port)
    
    self.ws_cloud_intent = websocket.WebSocketApp(cloud_intent_websocket_url,
                                    on_message = self.on_cloud_intent_message,
                                    on_error = self.on_error,
                                    on_close = self.on_close)
    
    self.ws_cloud_intent.on_open = self.on_open_cloud_intent
    self.ws_cloud_intent.keep_running = True 

    #Seperate background thread, to handle incoming messages
    self.ws_cloud_intent = threading.Thread(target=self.ws_cloud_intent.run_forever)
    self.ws_cloud_intent.daemon = True 
    self.ws_cloud_intent.start()
    
  def close_intent_socket(self):
    self.ws_cloud_intent.keep_running = False
    return

  def on_open_cloud_intent(self, ws):
    self.subscribeData["module"] = "cloudintents"
    subscribe_message = json.dumps(self.subscribeData)
    self.logger.info("Now waiting for a reply")
    ws.send(subscribe_message)
    self.cloud_intent_socket_established = True

  #For now, this only grabs the intent's function name that was triggered
  def on_cloud_intent_message(self, ws, message):
    if self.cloudIntent_stream_flag == True:
      self.logger.debug("Received a cloud intent message")
      message_to_json = json.loads(message)
    try:
      cloud_intent_name = message_to_json["data"]["intent"])
      self.cloudIntent_data_raw_results.append(cloud_intent_name)
    except:
      self.logger.debug("Cloud intent wasn't valid")
  
  #Eventually, this will process the entire returned JSON message, rather 
  #Than just the intent function name
  def process_and_return_cloud_intent(self):
    result = self.cloud_intent_data_raw_results[:] #makes a copy
    self.cloud_intent_data_raw_results = [] #reset for next
    return result

  def on_error(self, ws, error):
    self.logger.info(error)

  def on_close(ws):
    self.logger.info("### closed ###")
  
  def logging(self):
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)
    console = logging.StreamHandler() #for now, print to just console
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    console.setFormatter(formatter)
    logger.addHandler(console)
    return logger
  
