#!/usr/bin/env bash
set -eu

# Where is this script?
SCRIPTDIR=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))           

# What do we want to profile?
: ${ANKI_PROFILE_PROCNAME:="cozmoengined"}

# How long do we capture? (in seconds)
: ${ANKI_PROFILE_DURATION:="10"}

# How often do we sample? (in usec)
: ${ANKI_PROFILE_FREQUENCY:="4000"}

# Where is the symbol cache?
: ${ANKI_PROFILE_SYMBOLCACHE:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}/symbol_cache"}

# Where is the binary cache?
: ${ANKI_PROFILE_BINARYCACHE:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}/binary_cache"}

# Where is perf.data?
: ${ANKI_PROFILE_PERFDATA:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}/perf.data"}

# Where is the generated report?
: ${ANKI_PROFILE_REPORTDIR:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}"}

# Where is top level?
: ${TOPLEVEL:="`git rev-parse --show-toplevel`"}

# Where is simpleperf?
: ${SIMPLEPERF:="${TOPLEVEL}/lib/util/tools/simpleperf"}

#
# Create symbol cache
#
if [ ! -d ${ANKI_PROFILE_SYMBOLCACHE} ] ; then
  bash ${SCRIPTDIR}/make_symbol_cache.sh ${ANKI_PROFILE_SYMBOLCACHE}
fi

#
# Run report_html.py to start profiling.
# When it finishes it will pull a `perf.data` file off the robot.
#
# Use '-i' because perf.data is stored in a per process directory.
# Use '--symfs' because the symbolcache is per process.
# Use '-o' because reports are per process.
#

python ${SIMPLEPERF}/report_html.py \
  -i ${ANKI_PROFILE_PERFDATA} \
  --symfs ${ANKI_PROFILE_SYMBOLCACHE} \
  -o ${ANKI_PROFILE_REPORTDIR}/report.html  $@
open ${ANKI_PROFILE_REPORTDIR}/report.html
