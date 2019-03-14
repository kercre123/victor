from concurrent import futures

import cv2
import grpc
import logging
import json
import numpy as np
import time

import chipperpb_pb2
import chipperpb_pb2_grpc


class SnapperServicer(chipperpb_pb2_grpc.ChipperGrpcServicer):

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
    # One day this will be a real payload
    fake_payload = {
      'objects': [
        {
          'bounding_poly':
            [
              {'x': 0.01023531, u'y': 0.0017647222},
              {u'x': 0.89, u'y': 0.0017647222},
            ],
          'name': u'Person',
          'score': 0.9962483,
        },
      ],
    }

    image_response = chipperpb_pb2.ImageResponse()

    # Fill in the image result. This has the data
    # that is actually returned to OffboardModel.
    image_result = image_response.image_results.add()
    if len(request.modes) != 0:
      image_result.mode = request.modes[0]
    else:
      # TODO do we want to default our mode to LOCATE_OBJECT,
      # right now we're not setup to handle multiple requests
      # in this fashion it's handled by storing everything in
      # json.
      image_result.mode = 2

    image_result.raw_result = json.dumps(fake_payload)

    # Now make the rest of image response
    image_response.session = request.session
    image_response.device_id = request.device_id
    # TODO After going through the go client that
    # sits inbetween this and OffboardModel. Afaik
    # this isn't used we should probably not send
    # it; leaving it here for the moment for completeness.
    image_response.raw_result = json.dumps(fake_payload)

    return image_response

  def CreatePerson(self, request, context):
    raise NotImplementedError('Got CreatePerson request.')
  def CreatePersonGroup(self, request, context):
    raise NotImplementedError('Got CreatePersonGroup request.')
  def ListPersonGroup(self, request, context):
    raise NotImplementedError('Got ListPersonGroup request.')
  def DeletePersonGroup(self, request, context):
    raise NotImplementedError('Got DeletePersonGroup request.')
  def TrainPersonGroup(self, request, context):
    raise NotImplementedError('Got TrainPersonGroup request.')

def serve():
  server = grpc.server(futures.ThreadPoolExecutor(max_workers=4))
  chipperpb_pb2_grpc.add_ChipperGrpcServicer_to_server(
    SnapperServicer(), server)
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
