

all:
	make -C ./src
	make -C ./tests

clean:
	make -C ./src clean
	make -C ./tests clean
