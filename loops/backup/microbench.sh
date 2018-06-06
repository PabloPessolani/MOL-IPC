#/bin/bash
dmesg -c
./loop_mq_perf > result.txt
sleep 5

./loop_mq2_perf >> result.txt
sleep 5
./loop_mq3_perf >> result.txt
sleep 5

./loop_rs_perf >> result.txt
sleep 5
./loop_sr2_perf >> result.txt
sleep 5
./loop_nr2_perf >> result.txt
sleep 5

./loop_mq_copy >> result.txt
sleep 5
./loop_mq3_copy >> result.txt
sleep 5
./loop_sr3_copy >> result.txt
sleep 5
./loop_sr4_copy >> result.txt
sleep 5
