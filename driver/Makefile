MODNAME := sbd
obj-m := ${MODNAME}.o 
${MODNAME}-y :=
${MODNAME}-y += module.o socket.o httpdisk.o

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
