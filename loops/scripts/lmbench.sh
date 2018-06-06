#/bin/bash
rm loop_bw_mem*.txt

for j in  512 1024 2048 4096 8192 16384 32768 65536
do
	/usr/lib/lmbench/bin/i686-pc-linux-gnu/bw_mem $j bcopy  >bw_mem_$j.txt 2> bw_mem2_$j.txt
done
/usr/lib/lmbench/bin/i686-pc-linux-gnu/bw_pipe > bw_pipe_64K.txt
/usr/lib/lmbench/bin/i686-pc-linux-gnu/bw_unix > bw_unix.txt
/usr/lib/lmbench/bin/i686-pc-linux-gnu/cache > cache.txt
/usr/lib/lmbench/bin/i686-pc-linux-gnu/line > line.txt


