Nous sommes la Karparmy

Pour faire fonctionner le 1er driver : 

1) make 

2) insmod module.ko

3) mknod /dev/testdevice c 250 0

4) gcc -o appbuffer appbuffer.c

5) ./appbuffer 

6) enjoy
