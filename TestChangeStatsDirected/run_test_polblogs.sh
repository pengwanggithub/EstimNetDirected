#!/bin/sh

# Regression test for two-path matrix construction/update and change
# statistics.

BASELINE=polblogs_test_results_baseline.txt
OUTPUT=polblogs_test_results.out
DIFFILE=polblogs_test_results.diff

time ./testChangeStatsDirected ../pythonDemo/polblogs/polblogs_arclist.txt  polblogs_nodepairs.txt | fgrep -v nnz | fgrep -v DEBUG  > ${OUTPUT}

diff ${BASELINE} ${OUTPUT} > ${DIFFILE}

if [ $? -eq 0 ]; then
  echo
  echo "PASSED"
  exit 0
else 
  echo
  echo "**** FAILED ****"
  echo "diff results are in ${DIFFILE}"
  exit 1
fi

