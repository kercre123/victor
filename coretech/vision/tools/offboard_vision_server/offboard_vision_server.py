from concurrent import futures	

import cv2	
import grpc	
import logging	
import json	
import numpy as np	
import time	

import vision_pb2
import vision_pb2_grpc


class OffboardVisionServicer(vision_pb2_grpc.OffboardVisionGrpcServicer):

   def AnalyzeImage(self, request, context):	
    """	
    This is currently a catch all function for	
    getting an image from the OffboardModel in	
    coretech Neural Nets. It will decode the image	
    (which is a jpeg), and then return a fake payload	
    of a reponse that the OffboardModel is expecting.	
    """	

     # Decode the image	
    data = np.array(bytearray(request.image_data), dtype=np.uint8)	
    img = cv2.imdecode(data, cv2.IMREAD_UNCHANGED)	
    print('Succesfully decode an image of shape {}'.format(img.shape))	

     # Make the image response	
    # One day this will be a real payload.	
    # The color of the rectangle that will displayed	
    # on vector's face during mirror mode is determined	
    # by the order in this list.	
    fake_payload = {	
      'procType': 'ObjectDetection',	
      'objects': [	
        {	
          'bounding_poly':	
            [	
              {'x': 0.2, 'y': 0.25},	
              {'x': 0.4, 'y': 0.25},	
              {'x': 0.4, 'y': 0.75},	
              {'x': 0.2, 'y': 0.75}	
            ],	
          'name': 'Cat',	
          'score': 0.875	
        }, 	
        {	
          'bounding_poly':	
            [	
              {'x': 0.6, 'y': 0.25},	
              {'x': 0.8, 'y': 0.25},	
              {'x': 0.8, 'y': 0.75},	
              {'x': 0.6, 'y': 0.75}	
            ],	
          'name': 'Dog',	
          'score': 0.65	
        }	
      ]	
    }	

    image_response = vision_pb2.ImageResponse()
    image_response.raw_result = json.dumps(fake_payload)
    image_response.session = request.session
    image_response.device_id = request.device_id
    image_response.timestamp_ms = request.timestamp_ms

    return image_response	


def serve():
  server = grpc.server(futures.ThreadPoolExecutor(max_workers=4))
  vision_pb2_grpc.add_OffboardVisionGrpcServicer_to_server(
    OffboardVisionServicer(), server)
  server.add_insecure_port('[::]:16643')
  server.start()
  try:
    while True:
      time.sleep(3600)
  except KeyboardInterrupt:
    server.stop(0)

if __name__ == '__main__':	
  logging.basicConfig()	
  serve()
