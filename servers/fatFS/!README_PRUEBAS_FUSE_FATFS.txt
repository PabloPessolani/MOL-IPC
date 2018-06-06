Directorios Importantes

Lugar donde estan los scripts. Para probar todo sin info de debug (estan desactivadas todas las macros y todos los logs
porque se hacen muy voluminosos y cortan los procesos de copia de los archivos grandes sobre todo).

Arrancar todos los procesos, tal cual se hace siempre

root@node0:/home/MoL_Module/mol-ipc# ./fatfs.sh 0

Directorio donde estan los binarios de los servers incluido FATFS
/home/MoL_Module/mol-ipc/servers
							*---/fatFS
							*---/rs
							*---/pm
							*---/diskImgs

Directorio donde estan los binarios de FUSE que conectan con FATFS
/home/MoL_Module/mol-ipc/fatFuse

Directorio donde se encuentran las librerias de minix
/usr/src/linux-2.6.32/kernel/minix

Directorio donde se encuentra la MOLLIB usada
/home/MoL_Module/mol-ipc/lib/mollib


En el directorio /diskImgs se encuentran los archivos de las imagenes FAT 
* de diskette "floppy3FAT*.img"

como tambien la imagen de disco grande con archivos de todos los tamaños para probar
* "bigfat.img"

Esta ultima tiene los archivos de diferentes tamaños para probar

1Kb.txt   
2Kb.txt   
4Kb.txt   
8Kb.txt   
10Kb.txt  
500Kb.txt 
1Mb.txt   
5Mb.txt   
10Mb.txt   
20Mb.txt   
50Mb.txt   
100Mb.txt   
500Mb.txt 

Pasos para crear una imagen tipo bigfat.img, luego formatearla en fat y luego cargarle archivos de diferentes tamaños dentro.

CREACION DE UN ARCHIVO DE 500Mb
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# dd if=/dev/zero of=bigfat.img bs=1024k count=500
500+0 records in
500+0 records out
524288000 bytes (524 MB) copied, 5.99257 s, 87.5 MB/s

FORMATEARLO EN FAT
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# mkfs.vfat bigfat.img
mkfs.vfat 3.0.9 (31 Jan 2010)

CREACION DE UN ARCHIVO DE 1000Mb
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# dd if=/dev/zero of=bigfat.img bs=1024k count=1000
1000+0 records in
1000+0 records out
1048576000 bytes (1.0 GB) copied, 8.23496 s, 127 MB/s

FORMATEARLO EN FAT
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# mkfs.vfat bigfat.img      mkfs.vfat 3.0.9 (31 Jan 2010)

CREACION DE UN ARCHIVO DE 1500Mb
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# dd if=/dev/zero of=bigfat.img bs=1024k count=1500
1500+0 records in
1500+0 records out
1572864000 bytes (1.6 GB) copied, 12.5333 s, 125 MB/s

FORMATEARLO EN FAT
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# mkfs.vfat bigfat.img      mkfs.vfat 3.0.9 (31 Jan 2010)

MONTAR EL FS FAT EN /mnt
root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# mount bigfat.img /mnt/

root@node0:/home/MoL_Module/mol-ipc/servers/diskImgs# cd /mnt/
root@node0:/mnt# ls

CREARLE ARCHIVOS ADENTRO
root@node0:/mnt# time dd if=/dev/urandom of=1Mb.txt bs=1024k count=1
1+0 records in
1+0 records out
1048576 bytes (1.0 MB) copied, 0.232954 s, 4.5 MB/s

real    0m0.240s
user    0m0.000s
sys     0m0.234s

root@node0:/mnt# time dd if=/dev/urandom of=10Mb.txt bs=1024k count=10
10+0 records in
10+0 records out
10485760 bytes (10 MB) copied, 2.61991 s, 4.0 MB/s

real    0m2.622s
user    0m0.002s
sys     0m2.496s
root@node0:/mnt# ls
10Mb.txt  1Mb.txt
root@node0:/mnt# ls -la
total 11269
drwxr-xr-x  2 root root     4096 Aug 17 09:42 .
drwxr-xr-x 23 root root     1024 Jul  6  2016 ..
-rwxr-xr-x  1 root root 10485760 Aug 17 09:42 10Mb.txt
-rwxr-xr-x  1 root root  1048576 Aug 17 09:42 1Mb.txt

root@node0:/mnt# time dd if=/dev/urandom of=100Mb.txt bs=1024k count=100
100+0 records in
100+0 records out
104857600 bytes (105 MB) copied, 23.4417 s, 4.5 MB/s

real    0m23.444s
user    0m0.001s
sys     0m23.223s

root@node0:/mnt# time dd if=/dev/urandom of=500Mb.txt bs=1024k count=500
500+0 records in
500+0 records out
524288000 bytes (524 MB) copied, 126.36 s, 4.1 MB/s

real    2m6.362s
user    0m0.002s
sys     2m1.435s
root@node0:/mnt#


FORMA FACIL Y SIMPLE DE HACER ARCHIVOS DE TAMAÑO X CON CARACTERES LEGIBLES

base64 /dev/urandom | head -c $[1024*1024] > 1Mb.txt

base64 /dev/urandom | head -c $[1024*1024*50] > 50Mb.txt


/***********************************************************************************************************/
Pasos para probar FATFS+FUSE
/***********************************************************************************************************/

El FATFS, ya se encuentra configurado para que lea la imagen llamada bigfat.img, pero si se desea cambiar para
que lea otro archivo hay que editar el archivo:

/home/MoL_Module/mol-ipc/servers/fatFS/fatFS_VM0.cfg en la linea

# this is a comment 
# imagen local en archivo (usa read/write)
device MY_FILE_IMG {		
	major			1;
	minor			0;
	type			FILE_IMG;
	#filename 		"/home/MoL_Module/mol-ipc/servers/diskImgs/floppy3FAT_0.img";
	filename 		"/home/MoL_Module/mol-ipc/servers/diskImgs/bigfat.img";<----------ESTA LINEA
	volatile		NO;	
	root_dev		YES;
	buffer_size		4096;
};

Pasos para probar el funcionamiento de FATFS via FUSE con los comandos "more" y "cp"

1-) Una vez arrancado el script y seguidos todos los pasos en él hay que llegar hasta la ultima linea:

PM Enter para continuar...
RS Enter para continuar...
FATFS Enter para continuar...
VM p_nr -endp- -lpid- node flag misc -getf- -sndt- -wmig- -prxy- name
 0  -34    -34   2850    0    0   80  27342  27342  27342  27342 systask
 0   -2     -2   2849    0    8   20  31438  27342  27342  27342 systask
 0    0      0   2852    0    8   A0  31438  27342  27342  27342 pm
 0    1      1   2861    0    8   20  31438  27342  27342  27342 fatFS
 0    2      2   2855    0    0   20  27342  27342  27342  27342 rs
FATFS FUSE Enter para continuar...
VM p_nr -endp- -lpid- node flag misc -getf- -sndt- -wmig- -prxy- name
 0  -34    -34   2850    0    0   80  27342  27342  27342  27342 systask
 0   -2     -2   2849    0    8   20  31438  27342  27342  27342 systask
 0    0      0   2852    0    8   A0  31438  27342  27342  27342 pm
 0    1      1   2861    0    8   20  31438  27342  27342  27342 fatFS
 0    2      2   2855    0    0   20  27342  27342  27342  27342 rs
root@node0:/home/MoL_Module/mol-ipc#

Esta captura muestra los ultimos tres servers arrancados y el montaje de FATFS_FUSE.
El montaje se realiza (como está en el script, en el directorio /tmp/fu).

1) COMANDO MORE

Una vez hecho esto, se procede a probar el comando "more" de un archivo interior a la imagen bigfat.img

Por ejemplo
root@node0:/home/MoL_Module/mol-ipc# more /tmp/fu/1Kb.txt
dx/FfsqIS8J6WBAcUyyqvMEILaQ0LDlRnis8MA7IUL0ZecZQLRN8PlyueHjBc/OQ743AFwR6U2eY
D2ikA91SHgNI3SGRU6z3il1Mjizpws8KuwwOZVkM7hTuiv9xfeHeA6CbDq41r0pBgXq3k3hLw6Nb
hJGmfPq08UimS4pBiJDt4qDvO36j9QSwJUoMdHFcQpwf7aKX1Ni9uAzXIhf8RlytXUk2OgZe7tEu
+KiZoLdcMC9yaDhhBubNob+NGh1xD+jJ2SegZ/Fvwb1tD2sS4/vaBXGPlkcs8nFuywkevACMoCQO
GkbdKNsGxZcGYmZhpxDcSYyfxdH0kCAW7lunctK0mvZfdsJplsZbcFwJu+PmU5ebzb3daT1jOuTY
fx2/jAPCGeZ8d4ssTJDTvjqPzEWnl8gLETBMDKoC2T031O/n8iyFLWdsi8yqWoc9z06RvVQ+Fhea
rLhSaNoqpWogo8/QFfWcbbN22vGXfLe6ICE53qHMNu7z1q0tZZAw0KWYqoQFsjEQ/3R/EnlVz1Hu
yxrPdQ7/cVqk7HzC0B7ro/tRzZ/RSfSf+GBu3X1HyiJB1stDjH1IA5psupE5qGHyV+g/hJYCfDTa
pre8zFcWZbm96GA0semEa2zJvLH/Of2Yb1HZ3viRbLkAq/ydrfjr0M+pbUZ+4GX8NfRP4kxEdhw+
jKr36azUuHqqk4V7cxYqauBH42H0lpz+18421K7ysAGk3SW5gFOVW4WQQyW2VTVqGQ1natcmkt1f
/UQwZ2LQoWUgypb8jYJUU1OLl5Je4BYC1b69dgM/bmeBjaS6cB9rZq08g3RFrlQ8WoLdvFZ5VSWz
6C6YEnWjQslWtQPqD6lcoVpcVDuyOs2O115flBdMiAVsPMqNdVVqgnEtEQZ1gnNJeRyiW1tHsfcE
KIISikmvFiUXPDn3csoF+Ja5wKH64IZ/DKFbc0y3cQAXhssfPTNLyQRDND+LalMRv6UAoz/1hLIn
PpmnP/BgXf7J8UOxvBoqcwM
root@node0:/home/MoL_Module/mol-ipc# 

2) COMANDO COPY

root@node0:/home/MoL_Module/mol-ipc# cp /tmp/fu/10Mb.txt .

Luego de ejecutado este comando, se deberia contar con el archivo 10Mb.txt en el directorio local

root@node0:/home/MoL_Module/mol-ipc# ls
100Mb.txt             index.htm           randomtext.c
10Mb.txt <----ESTE!!! init                !README_PRUEBAS_FUSE_FATFS.txt
arch                  kernel              README.tests
BITACORA MOL-IPC.txt  lib                 rtest0.sh~
borrar.txt            loops               rtest.sh
catResult.txt         Makefile            servers
commands              minix_ip.sh         smallhog.c
coretemp[1].c         minixMOLFS_only.sh  spread.conf
cpu_disable.doc       minix.sh            spread.txt
dmesg0.txt            mol-buse            stub_syscall.c
dmesg1.txt            mol-fuse            stub_syscall.h
dmesg.txt             molFuse2            stub_syscall.o
error1.txt            molTests            tasks
error2.txt            molTestsLib         tcpdump.txt
fatfs0.sh             molTestsLibFAT      testcopy.c
fatfs.sh              mol_thread.h        tests
fatFS_test            moreResult.txt      thread_rs_mperf.c
fatfs_test.sh         node1.txt           threads.c
fatFuse               node2.txt           unieth.sh
fuseTests             procs.txt           unifat.sh
ifconfig.txt          proxy               unikernel.sh
include               randomtext
root@node0:/home/MoL_Module/mol-ipc#

Repetir las pruebas con time adelante para tener los tiempos de performance.


PARA CAMBIAR EL DRIVER (FILE_IMG o RDISK)

En el archivo fatFS_VM0.cfg de CONFIGURACION
está en /home/MoL_Module/mol-ipc/servers/fatFS

device MY_FILE_IMG {		
	major			1;
	minor			0;
	type			FILE_IMG;
	# filename 		"/home/MoL_Module/diskImgs/floppy3FAT_0.img";
	filename 		"/home/MoL_Module/diskImgs/bigfat.img";	
	volatile		NO;	
	root_dev		NO; <------ PONER YES en el driver a utilizar, los otros tienen que estar en NO
	buffer_size		4096;
};

# usa el driver rdisk
device MY_RDISK_IMG {
	major			3;
	minor			0;
	type			RDISK_IMG;
	image_file 		"";
	volatile		NO;	
	root_dev		YES; <------ PONER YES en el driver a utilizar, los otros tienen que estar en NO
	buffer_size		4096;
	compression 	NO;
};

ATENCION; si se usa el RDISK (que tiene su propio archivo de configuracion) hay que asegurarse que apunte a una imagen
válida en su configuracion:

El archivo de CONFIGURACION es rdisk3.cfg en este caso y esta en:
/home/MoL_Module/mol-ipc/tasks/rdisk


device MY_FILE_IMG {
	major			3;
	minor			0;
	type			FILE_IMAGE;
	image_file 		"/home/MoL_Module/diskImgs/bigfat.img"; <-----Misma ubicacion del archivo imagen que FATFS
	# image_file 	"/home/MoL_Module/mol-ipc/servers/diskImgs/floppy3FAT_0.img";
	# image_file 	"/home/MoL_Module/mol-ipc/tasks/rdisk/images/floppy3RWX.img";
#	image_file 		"/home/MoL_Module/mol-ipc/tasks/rdisk/images/tp4_img_modif.flp";
	volatile		NO;
	buffer			61440;
#	replicated		YES; /por ahora no se utiliza en el .cfg
};



