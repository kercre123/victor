#!/usr/bin/env bash
set -eu

# Where is this script?
SCRIPTDIR=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))           

# What do we want to profile?
: ${ANKI_PROFILE_PROCNAME:="vic-engine"}

# Where is the symbol cache?
: ${ANKI_PROFILE_SYMBOLCACHE:="${SCRIPTDIR}/symbol_cache"}

# Where is the binary cache?
: ${ANKI_PROFILE_BINARYCACHE:="${SCRIPTDIR}/binary_cache"}

# Where is perf.data?
: ${ANKI_PROFILE_PERFDATA:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}/perf.data"}

# Where are the annotated files?
: ${ANKI_PROFILE_ANNOTATEDDIR:="${SCRIPTDIR}/${ANKI_PROFILE_PROCNAME}/annotated_files"}

# Where is top level?
: ${TOPLEVEL:="`git rev-parse --show-toplevel`"}

# Where is simpleperf?
: ${SIMPLEPERF:="${TOPLEVEL}/lib/util/tools/simpleperf"}

# Where is addr2line?
: ${VICOS_ADDR2LINE:="`${TOPLEVEL}/project/victor/scripts/vicos_which.sh addr2line`"}

python ${SIMPLEPERF}/annotate.py \
  -o ${ANKI_PROFILE_ANNOTATEDDIR} \
  -i ${ANKI_PROFILE_PERFDATA} \
  -s ${TOPLEVEL} \
  --symfs ${ANKI_PROFILE_SYMBOLCACHE} \
  --addr2line ${VICOS_ADDR2LINE}
