 
аНбаЖаНб gcc, kernel-dev, linux-headers


1)
make


баЕаЗбаЛббаАб:
sergey@ubuntu:~/module$ make
make -C /lib/modules/2.6.38-15-generic/build M=/home/sergey/module modules
make[1]: Entering directory `/usr/src/linux-headers-2.6.38-15-generic'
  CC [M]  /home/sergey/module/testmodule.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/sergey/module/testmodule.mod.o
  LD [M]  /home/sergey/module/testmodule.ko
make[1]: Leaving directory `/usr/src/linux-headers-2.6.38-15-generic'




2)
sudo insmod ./testmodule.ko

3)
dmesg


4)
sudo rmmod ./testmodule.ko


5)
dmesg


[ 2602.639801] [test module] module's been loaded
[ 2614.629577] [test module] module's been unloaded
