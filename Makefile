CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Iinclude
SRC = src/sensor_filter.c src/sensor.c src/temperature_sensor.c src/demo.c
OUT = build/sensor_demo

all: $(OUT)

$(OUT): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: $(OUT)
	./$(OUT)

clean:
	rm -rf build

.PHONY: all run clean
