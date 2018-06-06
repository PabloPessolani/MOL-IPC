#/bin/bash
echo MOL > /var/log/daemon.log
dmesg -c
./drvs/drvsd 0 &
sleep 5
./tests/test_vm_init 0
./tests/test_add_node 0 1
./tests/test_rmtbind  0 11 1
./tests/test_rmtbind  0 12 1
./tests/test_rmtbind  0 13 1
./tests/test_rmtbind  0 14 1
./tests/test_rmtbind  0 15 1
./tests/test_rmtbind  0 16 1
./tests/test_rmtbind  0 17 1
./tests/test_rmtbind  0 18 1    
./tests/test_rmtbind  0 19 1
    
