#!/usr/bin/env bash
set -eu

# Where is this script?
SCRIPTDIR=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))           

# What do we want to profile?
: ${ANKI_PROFILE_PROCNAME:="vic-engine"}

# Where is symbol cache?
: ${ANKI_PROFILE_SYMBOLCACHE:="${SCRIPTDIR}/symbol_cache"}

# Where is perf.data?
: ${ANKI_PROFILE_PERFDATA:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}/perf.data"}

# Where is top level?
: ${TOPLEVEL:="`git rev-parse --show-toplevel`"}

# Where is simpleperf?
: ${SIMPLEPERF:="${TOPLEVEL}/lib/util/tools/simpleperf"}

#
# To view perf.data, run 
#  simpleperf report --symfs symbol_cache
# which will print performance stuff to console. 
#

export PATH=${SIMPLEPERF}/bin/darwin/x86_64:${PATH}
simpleperf report \
  -i ${ANKI_PROFILE_PERFDATA} \
  --symfs ${ANKI_PROFILE_SYMBOLCACHE} $@
