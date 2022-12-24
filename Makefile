build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc Round_Robin.c -o RR.out
	gcc PHPF.c -o PHPF.out
	gcc SRTN.c -o SRTN.out
	gcc SJF.c -o SJF.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out
