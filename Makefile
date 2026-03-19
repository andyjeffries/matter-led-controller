# PlatformIO CLI wrapper
# Usage: make flash PORT=/dev/cu.usbserial-XXX

MONITOR_SPEED = 115200
ENV = esp32dev

ifdef PORT
	PORT_FLAGS = --upload-port $(PORT)
	MONITOR_PORT_FLAGS = --port $(PORT)
endif

.PHONY: build upload monitor flash clean format devices

build:
	pio run -e $(ENV)

upload:
	pio run -e $(ENV) -t upload $(PORT_FLAGS)

monitor:
	pio device monitor -b $(MONITOR_SPEED) $(MONITOR_PORT_FLAGS)

flash: upload monitor

clean:
	pio run -e $(ENV) -t clean

format:
	clang-format -i src/*.cpp

devices:
	pio device list
