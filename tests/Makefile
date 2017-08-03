TARGETS = \
		  tests/armbe-basic/ \
		  tests/armle-basic/ \
		  tests/arm-to-thumb/ \
		  tests/thumb-to-arm/ \
		  tests/armle-func/ \
		  tests/helpers/ \
		  tests/thumb2/ \
		  tests/switch-table/

all: check

check: build
	cucumber -q && $(MAKE) clean

build:
	$(foreach dir,$(TARGETS),$(MAKE) -C $(dir) build;)

.PHONY: clean logclean

logclean:
	$(foreach dir,$(TARGETS),$(MAKE) -C $(dir) logclean;)

clean: logclean
	$(foreach dir,$(TARGETS),$(MAKE) -C $(dir) clean;)
