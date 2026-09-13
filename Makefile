# ============================================================
#  SkyWatch — Makefile
#  Wraps arduino-cli so the modular src/inc tree can be built
#  and flashed without opening the Arduino IDE.
#
#  Quick start:
#    make libs      # one-time: install ESP32 core + libraries
#    make build     # compile
#    make upload PORT=/dev/ttyUSB0
#    make fs        # build + flash the LittleFS data/ image
#
#  Requires: arduino-cli (https://arduino.github.io/arduino-cli)
# ============================================================

SKETCH_NAME  := SkyWatch
SKETCH_INO   := $(SKETCH_NAME).ino

# Board: Waveshare ESP32-S3-Knob-Touch-LCD-1.8.
# FlashSize/FlashMode/PSRAM below were confirmed against the physical device
# with `esptool flash_id` (16 MB quad-I/O flash, embedded octal PSRAM) — see
# ../SkyWatch_factory_backup/MANIFEST.txt. PartitionScheme=custom picks up
# partitions.csv in this folder (sized for that real 16 MB, not the board
# default's 4 MB — see partitions.csv for the full rationale).
# CDCOnBoot=cdc: the board default (Disabled) routes Arduino's Serial
# object to the physical UART0 pins instead of the USB port this board is
# actually flashed/monitored over — with it Disabled, every Serial.print()
# in the firmware goes nowhere reachable, only the ROM's own low-level boot
# messages appear over USB regardless of this setting. Confirmed by capturing
# raw serial after reset: without this, only ROM output showed up; the app
# never printed anything past that, even though it demonstrably keeps running
# (WiFi connects, knob works, etc.) — with CDCOnBoot=cdc, Serial.println()
# is what SkyWatch.ino actually calls throughout setup()/loop() for status.
# Re-verify option keys/values for your installed core with:
#   arduino-cli board details -b esp32:esp32:esp32s3 --full
FQBN ?= esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=custom,CDCOnBoot=cdc,PSRAM=opi,UploadSpeed=921600

# Serial port for upload/monitor — override on the command line, e.g.:
#   make upload PORT=/dev/cu.usbmodem14101
PORT ?= /dev/ttyUSB0
BAUD ?= 115200

# ── Module layout ───────────────────────────────────────────
# Every top-level module folder follows <module>/src/*.cpp + <module>/inc/*.h.
# Header-only modules (no src/) are simply skipped by the wildcard below.
MODULES := config flight airline display screen haversine \
           wifi_manager opensky records aircraft_photo knob screensaver touch

INC_DIRS   := $(foreach m,$(MODULES),$(CURDIR)/$(m)/inc)
INC_FLAGS  := $(foreach d,$(INC_DIRS),-I$(d))
MODULE_SRC := $(foreach m,$(MODULES),$(wildcard $(m)/src/*.cpp))

# ── Build paths ─────────────────────────────────────────────
BUILD_DIR     := build
# arduino-cli requires the sketch folder name to match the .ino basename,
# so the assembled copy must be named after the sketch, not "sketch".
BUILD_SKETCH  := $(BUILD_DIR)/$(SKETCH_NAME)
OUTPUT_DIR    := $(BUILD_DIR)/output

# arduino-cli only compiles sources that live directly inside the sketch
# folder, so we assemble a flat copy here on every build. Headers are NOT
# copied — they're found via INC_FLAGS, so each module's inc/ stays the
# single source of truth for its header. partitions.csv DOES need to be
# copied in: with PartitionScheme=custom, arduino-cli looks for it inside
# the sketch folder it's actually compiling (this assembled copy), not the
# project root — a custom table sitting only at the root is silently never
# picked up.
ASSEMBLED_SRC := $(BUILD_SKETCH)/$(SKETCH_INO) \
                  $(BUILD_SKETCH)/partitions.csv \
                  $(patsubst %,$(BUILD_SKETCH)/%,$(notdir $(MODULE_SRC)))

.PHONY: all help libs build upload monitor fs clean

all: build

help:
	@echo "SkyWatch build targets:"
	@echo "  make libs             Install ESP32 core + required libraries"
	@echo "  make build            Compile the firmware (build/output/*.bin)"
	@echo "  make upload PORT=...  Compile and flash firmware"
	@echo "  make fs                Build and flash the LittleFS data/ image"
	@echo "  make monitor PORT=...  Open serial monitor at $(BAUD) baud"
	@echo "  make clean             Remove build/"

# ── One-time environment setup ──────────────────────────────
libs:
	arduino-cli core update-index
	arduino-cli core install esp32:esp32
	arduino-cli lib install "GFX Library for Arduino"
	arduino-cli lib install "TJpg_Decoder"
	arduino-cli lib install "ArduinoJson"

# ── Assemble the flat build sketch ──────────────────────────
$(BUILD_SKETCH):
	@mkdir -p $(BUILD_SKETCH)

$(BUILD_SKETCH)/$(SKETCH_INO): $(SKETCH_INO) | $(BUILD_SKETCH)
	@cp $(SKETCH_INO) $(BUILD_SKETCH)/

$(BUILD_SKETCH)/partitions.csv: partitions.csv | $(BUILD_SKETCH)
	@cp partitions.csv $(BUILD_SKETCH)/

# Pattern rule: copy every module .cpp into the flat sketch dir.
# (Module basenames are unique across the tree, so flattening is safe.)
define COPY_SRC_RULE
$(BUILD_SKETCH)/$(notdir $(1)): $(1) | $(BUILD_SKETCH)
	@cp $(1) $(BUILD_SKETCH)/
endef
$(foreach f,$(MODULE_SRC),$(eval $(call COPY_SRC_RULE,$(f))))

# ── Compile ──────────────────────────────────────────────────
build: $(ASSEMBLED_SRC)
	arduino-cli compile \
	  --fqbn "$(FQBN)" \
	  --build-property "compiler.cpp.extra_flags=$(INC_FLAGS)" \
	  --build-property "compiler.c.extra_flags=$(INC_FLAGS)" \
	  --output-dir $(OUTPUT_DIR) \
	  --warnings default \
	  $(BUILD_SKETCH)

# ── Flash firmware ──────────────────────────────────────────
upload: build
	arduino-cli upload -p $(PORT) --fqbn "$(FQBN)" \
	  --input-dir $(OUTPUT_DIR) $(BUILD_SKETCH)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

# ── LittleFS data image (data/config.json etc.) ─────────────
# Locates mklittlefs and esptool inside the ESP32 core installed by
# `make libs`, builds a filesystem image from data/, and flashes it.
# Offset/size must match the "spiffs" row in partitions.csv exactly.
LITTLEFS_OFFSET ?= 0x410000
LITTLEFS_SIZE   ?= 0xbe0000
# arduino-cli's data dir is ~/.arduino15 on Linux/Windows but
# ~/Library/Arduino15 on macOS — search both, take the newest version found.
ARDUINO_DATA_DIRS := $(HOME)/.arduino15 $(HOME)/Library/Arduino15
MKLITTLEFS := $(shell find $(ARDUINO_DATA_DIRS) -path "*/tools/mklittlefs/*" -name mklittlefs -type f 2>/dev/null | sort -V | tail -n1)
ESPTOOL    := $(shell find $(ARDUINO_DATA_DIRS) -path "*/tools/esptool_py/*" -name "esptool*" -type f 2>/dev/null | sort -V | tail -n1)

fs:
	@test -n "$(MKLITTLEFS)" || (echo "mklittlefs not found — run 'make libs' first" && exit 1)
	@test -n "$(ESPTOOL)"    || (echo "esptool not found — run 'make libs' first" && exit 1)
	@mkdir -p $(BUILD_DIR)
	$(MKLITTLEFS) -c data -b 4096 -p 256 -s $(LITTLEFS_SIZE) $(BUILD_DIR)/littlefs.bin
	$(ESPTOOL) --chip esp32s3 -p $(PORT) -b 921600 write_flash \
	  $(LITTLEFS_OFFSET) $(BUILD_DIR)/littlefs.bin

# ── Housekeeping ─────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR)
