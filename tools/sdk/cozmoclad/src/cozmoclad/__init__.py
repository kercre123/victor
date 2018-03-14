# Copyright (c) 2016 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


__version__ = "2.5.0"


# build version string, to match the one in the app engine
__build_version__ = "00002.00005.00000"


class CLADHashMismatch(Exception):
    '''Raised by assert_clad_match if the supplied CLAD hashes do not match'''


def assert_hash_match(first, second, check_name):
    '''Compare two CLAD hashes for equality

    Args:
        first (int, list of ints or bytes): First hash to compare
        second (int, list of ints or bytes): Second hash to compare
        check_name (string): A description to put in a raised exception message
    Returns:
        True if the hashes match exactly
    Raises:
        CLADHashMismatch if the hashes do not match.
    '''
    def normalize(h):
        if isinstance(h, bytes):
            return h
        if isinstance(h, list):
            return bytes(h)
        if isinstance(h, int):
            return h.to_bytes(16, byteorder='little')
        raise TypeError('Invalid hash type %s for value %s' % (type(h), h))

    first = normalize(first)
    second = normalize(second)

    if first != second:
        raise CLADHashMismatch("CLAD version mismatch (%s) %s != %s" % (check_name, first.hex(), second.hex()))
    return True


def assert_clad_match(to_game_hash, to_engine_hash):
    '''Assert that the supplied CLAD hashes match those in this package.

    Args:
        to_game_hash (int, list of ints or bytes): The expected hash for the
            message engine to game interface
        to_engine_hash (int, list of ints or bytes): The expected hash for the
            message game to interface interface
    Returns:
        True if the hashes match exactly
    Raises:
        CLADHashMismatch if the hashes do not match.
    '''
    from .clad.externalInterface.messageEngineToGame_hash import messageEngineToGameHash as clad_to_game_hash
    from .clad.externalInterface.messageGameToEngine_hash import messageGameToEngineHash as clad_to_engine_hash

    assert_hash_match(to_game_hash, clad_to_game_hash, 'to_game')
    assert_hash_match(to_engine_hash, clad_to_engine_hash, 'to_engine')
    return True
