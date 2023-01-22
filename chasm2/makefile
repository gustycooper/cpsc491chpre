# makes chasm and charm_tools
# % make
# % make chasm
# % make clean
all: chasm

FLAGS = -Wall
chasm: chasm.c chasm_types.h fsms.c fsms.h dict.c dict.h defdict.c defdict.h charmopcodes.h
	@echo 'building chasm.'
	gcc -o chasm chasm.c fsms.c dict.c defdict.c $(FLAGS)

clean:
	rm -f chasm

