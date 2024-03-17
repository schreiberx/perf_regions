

all:	build_src

build_src:
	make -C ./src MODE=${MODE}

examples:
	make -C ./examples MODE=${MODE}

run_tests:
	make -C ./examples run_tests  MODE=${MODE}

clean:
	make -C ./src clean MODE=${MODE}
	make -C ./examples clean MODE=${MODE}
