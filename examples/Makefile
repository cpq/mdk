ARCHITECTURES	= esp32c3 esp32
EXAMPLES := $(dir $(wildcard */Makefile))

all: build clean

build:
	@for a in $(ARCHITECTURES) ; do \
		for d in $(EXAMPLES) ; do \
			make -C $$d clean build ARCH=$$a || exit 1; \
		done ; \
	done

clean:
	@for d in $(EXAMPLES) ; do make -C $$d clean ; done
