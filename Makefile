TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./$(TARGET) -f ./mynewdb.db -n
	./$(TARGET) -f ./mynewdb.db -a "Timmy H., 123 Sheshire Ln., 120"

default: $(TARGET)

clean:
	rm -rf obj
	rm -rf bin
	rm -f *.db

$(TARGET): $(OBJ)
	mkdir -p bin
	gcc -o $@ $?

obj/%.o : src/%.c 
	mkdir -p obj
	gcc -c $< -o $@ -Iinclude
