FILES=$(wildcard app/*.c)

main: $(FILES)
	$(CC) -ggdb -Wall $(FILES) -o main -lcurl -lz
