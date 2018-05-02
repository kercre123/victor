
import sys
if sys.version_info < (3,5,1):
    sys.exit('victor requires Python 3.5.1 or later')

from . import _clad
from . import robot

__all__ = ['robot']
