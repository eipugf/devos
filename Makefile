# obj-m is a list of what kernel modules to build.  The .o and other
# objects will be automatically built from the corresponding .c file -
# no need to list the source files explicitly.

obj-m := ph_memory.o 

#EXTRA_CFLAGS := -O2

# KDIR is the location of the kernel source.  The current standard is
# to link to the associated source tree from the directory containing
# the compiled modules.
#KDIR  :=/lib/modules/4.9.0-3-amd64/source
KDIR  := /usr/src/linux-headers-4.9.0-4-amd64

# PWD is the current working directory and the location of our module
# source files.
PWD   := $(shell pwd)

# default is the default make target.  The rule here says to run make
# with a working directory of the directory containing the kernel
# source and compile only the modules in the PWD (local) directory.
all: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules 
