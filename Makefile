

all:
	make -C ./src MODE=${MODE}
	make -C ./examples MODE=${MODE}

clean:
	make -C ./src clean MODE=${MODE}
	make -C ./examples clean MODE=${MODE}
