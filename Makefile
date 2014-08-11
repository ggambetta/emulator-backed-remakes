
all:
	make -C goody
	make -C tools

clean:
	make -C goody clean
	make -C tools clean
	make -C lib clean
