#!/bin/sh
#
# File:    estimnetdirectedEstimation2textableMultiModels.sh
# Author:  Alex Stivala
# Created: Februrary 2017
#
#
# Read output of computeEstimNetDirectedCovariance.R with the estimate,
# estimated std. error and t-ratio computed from EstimNetDirected results
# and build LaTeX table for multiple different models
# 
# Usage: estimnetdirectedEstimation2textableMultiModels.sh estimationoutputfile_model1 estimationoutputfile_model2 ...
#
# E.g.:
#   estimnetdirectedEstimation2textableMultiModels.sh  estimation.out model2/estimation.out
#
# Output is to stdout
#
# Uses various GNU utils options on echo, etc.

zSigma=2 # multiplier of estimated standard error for nominal 95% C.I.
tratioThreshold=0.3 # t-ratio must be <= this for significance

if [ $# -lt 1 ]; then
    echo "usage: $0 estimation1.out estimation2.out ..." >&2
    exit 1
fi

num_models=`expr $#`

estimnet_tmpfile=`mktemp`

echo "% Generated by: $0 $*"
echo "% At: " `date`
echo "% On: " `uname -a`

echo -n '\begin{tabular}{l'
i=1
while [ $i -le $num_models ]
do
  echo -n r
  i=`expr $i + 1`
done
echo '}'
echo '\hline'  
echo -n 'Effect '
i=1
while [ $i -le $num_models ]
do
  echo -n " & Model $i"
  i=`expr $i + 1`
done
echo '\\'
echo '\hline'  

# new version has 6 columns:
# Effect   estimate   sd(theta)   est.MLE.std.err.   est.std.err  t.ratio
# (and maybe *) plus
# TotalRuns and ConvergedRuns e.g.:
#Diff_completion_percentage -0.003543378 0.004887503 8.481051e-05 0.004972314 0.01630916
#TotalRuns 2
#ConvergedRuns 2
# (see computeEstimNetDirectedCovariance.R)
model=1
for estimationresults in $*
do
    cat ${estimationresults}  | tr -d '*' | fgrep -vw AcceptanceRate | fgrep -vw TotalRuns | fgrep -vw ConvergedRuns | awk '{print $1,$2,$5,$6}'  |  tr ' ' '\t' | sed "s/^/${model}\t/" >> ${estimnet_tmpfile}
    model=`expr $model + 1`
done


effectlist=`cat ${estimnet_tmpfile} |  awk '{print $2}' | sort | uniq`

for effect in ${effectlist}
do
  model=1
  echo -n "${effect} " | tr '_' ' '
  while [ $model -le $num_models ]; 
  do
    estimnet_point=`grep -w ${effect} ${estimnet_tmpfile} | awk -vmodel=$model '$1 == model {print $3}'`
    estimnet_stderr=`grep -w ${effect} ${estimnet_tmpfile} | awk -vmodel=$model '$1 == model {print $4}'`
    estimnet_tratio=`grep -w ${effect} ${estimnet_tmpfile} | awk -vmodel=$model '$1 == model {print $5}'`
    if [ "${estimnet_point}" == "" ];  then
      echo -n " & ---"
    else 
      # put statistically significant results in normal text, others
      # in "\light" which must be defined as e.g.
      # \newcommand{\light}[1]{\textcolor{gray}{#1}}
      # which requires \usepackage{xcolor} (NB color package does not have gray)
      
      # bc cannot handle scientific notation so use sed to convert it 
      estimnet_lower=`echo "${estimnet_point} - ${zSigma} * ${estimnet_stderr}" | sed -e 's/[eE]+*/*10^/' | bc -l`
      estimnet_upper=`echo "${estimnet_point} + ${zSigma} * ${estimnet_stderr}" | sed -e 's/[eE]+*/*10^/' | bc -l`
      estimnet_point=`echo "${estimnet_point}" | sed -e 's/[eE]+*/*10^/'`
      estimnet_tratio=`echo "${estimnet_tratio}" | sed -e 's/[eE]+*/*10^/'`
      estimnet_stderr=`echo "${estimnet_stderr}" | sed -e 's/[eE]+*/*10^/'`
      echo AAA "${estimnet_point}">&2
      abs_estimate=`echo "if (${estimnet_point} < 0) -(${estimnet_point}) else ${estimnet_point}" | bc -l`
      abs_tratio=`echo "if (${estimnet_tratio} < 0) -(${estimnet_tratio}) else ${estimnet_tratio}" | bc -l`
      echo YYY ${abs_estimate} >&2
      echo QQQ ${abs_tratio} >&2
      echo XXX "${abs_tratio} <= ${tratioThreshold} && ${abs_estimate} > ${zSigma} * ${estimnet_stderr}" >&2
      signif=`echo "${abs_tratio} <= ${tratioThreshold} && ${abs_estimate} > ${zSigma} * ${estimnet_stderr}" | bc -l`
      echo ZZZ ${signif} >&2 
      if [ ${signif} -eq 0 ]; then
        printf ' & $\\light{\\underset{(%.3f, %.3f)}{%.3f}}$' ${estimnet_lower} ${estimnet_upper} ${estimnet_point}
      else
        printf ' & $\\underset{(%.3f, %.3f)}{%.3f}$' ${estimnet_lower} ${estimnet_upper} ${estimnet_point}
      fi
    fi
    model=`expr $model + 1`
  done
  echo '\\'
done

echo '\hline'  
echo '\end{tabular}'

rm ${estimnet_tmpfile}
