Installing and using M3-IPC
============================

WARNING: Current version of M3-IPC is a patch, a library and a set of sample programs 
We have finished a new version based on a kernel loadable module, but it is in testing stage.

M3-IPC was developed and tested on Linux Debian 6.0.4 with kernel 2.6.32

Download the patch ( m3-ipc.diff)
Apply the patch 
	cd /usr/src/linux
	patch -p1 < m3-ipc.diff

Download libraries and sample programs m3-ipc.tar.gz

create a user jara then:
cd /home/jara
tar xvzf m3-ipc.tar.gz

it creates a mol-ipc directory with serveral subdirectories
test: tests programs 
loops: performance microbenchmarks
proxy: proxy processes
proxy/loops: performance microbenchmarks for remote IPC.

rlen	nr record	filesize
64	16777216	1073741824



