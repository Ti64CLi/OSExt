



all: installer kernel
	cp kernel/osext.tns installer/installer/boot/osext.tns
	

.PHONY: installer kernel remake
	

installer:
	$(MAKE) -C installer


kernel:
	$(MAKE) -C kernel



clean:
	$(MAKE) -C installer clean && \
	$(MAKE) -C kernel clean && \
	rm installer/installer/boot/osext.tns


remake:
	$(MAKE) clean && \
	$(MAKE) all