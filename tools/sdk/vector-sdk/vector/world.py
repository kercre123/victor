# Copyright (c) 2018 Anki, Inc.

'''
The "world" represents the robot's known view of its environment.
It keeps track of all the faces Vector has observed.
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['World']

from vector import faces


class World:
    '''Represents the state of the world, as known to Vector.'''

    #: callable: The factory function that returns a
    #: :class:`faces.Face` class or subclass instance.
    face_factory = faces.Face

    def __init__(self):
        self._faces = {}

    @property
    def visible_faces(self):
        '''generator: yields each face that Vector can currently see.
        
        Returns:
            A generator yielding :class:`vector.faces.Face` instances
        '''
        for face in self._faces.values():
            yield face
    
    def get_face(self, face_id):
        '''vector.faces.Face: Fetch a Face instance with the given id'''
        return self._faces.get(face_id)

    def add_update_face_to_world_view(self, _, msg):
        '''Adds/Updates the world view when a face is observed'''
        face = self.face_factory()
        face.unpack_face_stream_data(msg)
        self._faces[face.face_id] = face

    def update_face_id(self, _, msg):
        '''Updates the face id when a tracked face (negative ID) is recognized and 
        receives a positive ID or when face records get merged'''
        face = self.get_face(msg.old_id)
        if face:
            face._updated_face_id = msg.new_id