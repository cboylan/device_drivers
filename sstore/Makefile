### Clark Boylan
### CS 572
### Homework 1
### sstore driver
### Makefile
### 11/03/2010

### Automatic Makefile generation by 'genmake' script        ####
### Copyright, Jerry Cooperstein, coop@linuxfoundation.org 2/2003 - 11/2009 ####
### License: GPLv2 ###

obj-m +=  sstore.o

export KROOT=/lib/modules/2.6.24.7-test2/build

allofit:   modules sstore_test

sstore_test: sstore_test.c
	gcc sstore_test.c -o sstore_test

modules:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules
modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install
kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(PWD) clean

clean: kernel_clean
	rm -rf   Module.symvers modules.order sstore_test
