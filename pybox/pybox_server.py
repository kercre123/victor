from concurrent import futures

import grpc
import logging
import time

import chipperpb_pb2
import chipperpb_pb2_grpc

class SnapperServicer(chipperpb_pb2_grpc.ChipperGrpcServicer):

	def AnalyzeImage(self, request, context):
		# request is chipperpb_pb2_grpc.ImageRequest
		# should return chipperpb_pb2_grpc.ImageResponse
		# TODO: do something with request
		raise NotImplementedError('got request!')

	# TODO: implement if desired
	# def CreatePerson(self, request, context):
	# 	pass
	# def CreatePersonGroup(self, request, context):
	# 	pass
	# def ListPersonGroup(self, request, context):
	# 	pass
	# def DeletePersonGroup(self, request, context):
	# 	pass
	# def TrainPersonGroup(self, request, context):
	# 	pass

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
