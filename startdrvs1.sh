#/bin/bash
echo MOL > /var/log/daemon.log
dmesg -c
./drvs/drvsd 1 &
sleep 5
./tests/test_vm_init 0
./tests/test_add_node 0 0
./tests/test_rmtbind  0 1 0
./tests/test_rmtbind  0 2 0
./tests/test_rmtbind  0 3 0
./tests/test_rmtbind  0 4 0
./tests/test_rmtbind  0 5 0
./tests/test_rmtbind  0 6 0
./tests/test_rmtbind  0 7 0
./tests/test_rmtbind  0 8 0
./tests/test_rmtbind  0 9 0



     
    
