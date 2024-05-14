FILES=$(wildcard app/*.c)

main: $(FILES)
	gcc -ggdb -Wall $(FILES) -o main -lcurl 
