#!/bin/bash
perf stat /home/jara/mol-ipc/loops/loop_mol_ipc1 1000000 2> perf_ipc1.txt
perf stat /home/jara/mol-ipc/loops/loop_mol_ipc2 1000000 2> perf_ipc2.txt
perf stat /home/jara/mol-ipc/loops/loop_mol_ipc3 1000000 2> perf_ipc3.txt
perf stat /home/jara/mol-ipc/loops/loop_mol_zcopy1 1000000 4096 2> perf_zcopy1.txt
perf stat /home/jara/mol-ipc/loops/loop_mol_zcopy2 1000000 4096 2> perf_zcopy2.txt
