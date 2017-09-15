#!/bin/bash

iter=0
failed=0

while [ $failed -ne 1 ] ; do
    echo -n "Iteration $iter"
    tf=`printf "tasks_iter_%06d.yaml" $iter`
    cf=`printf "comps_iter_%06d.yaml" $iter`
    pf=`printf "plan_out_%06d.txt" $iter`
    test/make_tasks_comp.py --num-tasks=10 --num-compute=4 --compute-file=$cf --task-file=$tf
    src/planner --tasks $tf --compute $cf --analyze --verbose > $pf
    rv=$?
    if [ $rv -ne 0 ] ; then
        echo " failed: $rv"
        failed=1
    else
        echo " passed: $rv"
        rm $pf $tf $cf
    fi
    iter=$((iter + 1))
done

