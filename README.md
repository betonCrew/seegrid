seegrid
================================
a floating lighting installation

# Electronics
## Comms
- rf
- infrared ([pixmob](https://github.com/danielweidman/pixmob-ir-reverse-engineering/tree/main))
- wire

---

### 📡 **1. nRF24L01+ (2.4 GHz)**
- **Interface:** SPI
- **Range:** Up to 100 m (PA/LNA versions even more)
- **Power:** ~11.3 mA TX, ~13.5 mA RX, 900 nA in standby
- **Pros:**
  - Super cheap (< $1)
  - Libraries available (e.g., [RF24](https://tmrh20.github.io/RF24/))
  - Star or point-to-point topology
- **Cons:**
  - Not Bluetooth/Zigbee-compatible
  - No encryption unless you add it yourself

💡 **Great for low-latency, low-power remote control systems.**

---

### 🐣 **2. RFM69 (868/915 MHz)**
- **Interface:** SPI
- **Range:** 500–1000 m (with decent antennas)
- **Power:** ~16–45 mA TX (configurable), <1 µA sleep
- **Pros:**
  - Sub-GHz = better wall penetration
  - AES encryption built-in
  - Easy libraries (e.g., RadioHead)
- **Cons:**
  - Slightly more expensive (~$2)
  - Slower data rates than 2.4 GHz

💡 **Best for battery-powered lights needing longer range.**

---

### 🌲 **3. SX1276/SX1278 (LoRa, 433/868/915 MHz)**
- **Interface:** SPI
- **Range:** 2–5 km LoS (crazy long range)
- **Power:** ~10–120 mA TX, ~10 mA RX, ~1 µA sleep
- **Pros:**
  - LoRa modulation = insane range
  - AES encryption
- **Cons:**
  - Not designed for real-time control (long packet times)
  - More expensive (~$3–4)
  - Lower data rate

💡 Use if you want to light up a barn 2km away on a button press. 🌾

---

### 🌐 **4. ESP-NOW (via ESP8266/ESP32 as radio co-processor)**
- **Interface:** UART/SPI to ESP
- **Power:** Depends on config (ESP in light sleep)
- **Pros:**
  - ESP-NOW is fast, simple, low-latency 2.4 GHz protocol
  - No need for full Wi-Fi stack
  - Use a cheap ESP-01 module just as a “wireless chip”
- **Cons:**
  - ESPs are power-hungry in comparison unless you actively manage sleep
  - Bit of overhead vs pure RF modules

💡 Hackable option if you already have ESP modules lying around.

---

### 📶 **5. CC1101 (Sub-GHz, 433/868 MHz)**
- **Interface:** SPI
- **Range:** 300–800 m
- **Power:** ~14–25 mA TX, ~16 mA RX, <1 µA sleep
- **Pros:**
  - TI-quality sub-GHz transceiver
  - Very stable, good open libraries
- **Cons:**
  - Slightly tricky to configure, but great once running

💡 Excellent for industrial-grade point-to-point lighting control

---

### 💬 Summary Table:

| Module        | Freq     | Range     | Power Use         | Protocol       | Price | Notes |
|---------------|----------|-----------|--------------------|----------------|-------|-------|
| **nRF24L01+**  | 2.4 GHz  | ~100 m    | ~13 mA active      | Proprietary    | <$1   | Fast, easy |
| **RFM69**      | 868/915  | ~500–1000 m| ~16–45 mA TX       | Proprietary    | ~$2   | AES, sub-GHz |
| **SX127x (LoRa)**| 433/868| ~2–5 km   | ~120 mA TX         | LoRa           | ~$3–4 | Long range, low data |
| **ESP-01 (ESP-NOW)** | 2.4 GHz | ~100 m | ~70–150 mA active | ESP-NOW        | ~$1.50 | Soft-radio, flexible |
| **CC1101**     | 433/868  | ~800 m    | ~14–25 mA TX       | Proprietary    | ~$2   | TI quality |


---

### 🧱 **1. RFM69 Series** (FSK/OOK, general-purpose sub-GHz)
- **Modulation:** FSK/GFSK/OOK
- **Frequencies:** 315, 433, 868, 915 MHz (region-dependent)
- **Features:**
  - Built-in **AES-128 encryption**
  - Packet-based communication
  - Reliable and easy to use (Arduino, RadioHead, MySensors, etc.)
- **Modules:**
  | Module      | Frequency | Output Power | Notes |
  |-------------|-----------|--------------|-------|
  | **RFM69CW** | 433/868/915 MHz | +13 dBm     | Low power, compact |
  | **RFM69HCW**| 433/868/915 MHz | +20 dBm     | "High power" version |
  | **RFM69W**  | 433/868/915 MHz | +13 dBm     | Legacy, similar to CW |
  | **RFM69HW** | 433/868/915 MHz | +20 dBm     | High-power legacy |
- **Use case:** General wireless control, home automation, MySensors

---

### 🦙 **2. RFM9x Series** (LoRa modulation, long range)
- **Modulation:** LoRa (spread-spectrum)
- **Frequencies:** 433, 868, 915 MHz
- **Features:**
  - Long range (up to several km)
  - Lower power efficiency for fast messaging
- **Modules:**
  | Module      | Frequency | Output Power | Notes |
  |-------------|-----------|--------------|-------|
  | **RFM95W**  | 868/915 MHz | +20 dBm     | LoRa, popular in TTN |
  | **RFM96W**  | 433 MHz    | +20 dBm     | 433 MHz LoRa |
  | **RFM98W**  | 433 MHz    | +20 dBm     | LoRa, long-range focused |
- **Use case:** LoRaWAN, point-to-point over long distance

---

### 📡 **3. RFM22/23 Series** (FSK/OOK, packet-based, older generation)
- **Modulation:** FSK/GFSK/OOK
- **Frequencies:** 433, 868, 915 MHz
- **Features:**
  - SPI-controlled, pre-LoRa era
  - Similar to RFM69 but older
- **Modules:**
  | Module      | Frequency | Output Power | Notes |
  |-------------|-----------|--------------|-------|
  | **RFM22B**  | 433/868/915 MHz | +20 dBm | Compatible with Si4432 |
  | **RFM23BP** | 433/868/915 MHz | +30 dBm | High-power version |
- **Use case:** Legacy wireless links; still usable but eclipsed by RFM69

---

### 🧙‍♂️ **4. RFM12B / RFM01 / RFM02** (Obsolete or minimal)
- **Modulation:** OOK/FSK
- **Features:** Very simple modules, used in early DIY sensors
- **Status:** Mostly obsolete
- **Modules:**
  - **RFM12B**: Was very popular in OpenEnergyMonitor, Jeelib, etc.
  - **RFM01/02**: RX/TX only (no transceiver)

💡 **Only recommended if maintaining legacy designs**

---

### 🧩 Summary Table

| Series    | Modulation     | Main Use Case         | Power Output     | Notes |
|-----------|----------------|------------------------|------------------|-------|
| RFM69     | FSK/GFSK/OOK   | General low-power RF   | +13 to +20 dBm   | Easy to use, AES support |
| RFM9x     | LoRa           | Long range, lower throughput | +20 dBm    | LoRa/LoRaWAN, remote sensors |
| RFM22/23  | FSK            | Legacy, still usable   | +20 to +30 dBm   | High power, old gen |
| RFM12B    | FSK            | Legacy hobby projects  | ~+10 dBm         | Obsolete, minimal |
| RFM01/02  | OOK/FSK (RX/TX only) | Very simple one-way comms | Varies   | Very limited, minimal control |

---

## MCU

### 🔧 **1. ESP32 (Wi-Fi + Bluetooth)**
- **Pros:**
  - Integrated Wi-Fi **and** Bluetooth (BLE)
  - Dual-core, fast enough for animations and effects
  - Very cheap (~$2-4 for modules like ESP32-WROOM-32)
  - Wide community support and lots of libraries (e.g., for FastLED / NeoPixel)
  - Deep sleep modes for power saving
- **Cons:**
  - Higher power draw when Wi-Fi is active (~100-200 mA), but deep sleep can go below 10 µA

💡 Best for: **Wi-Fi control via phone/web app**, OTA updates, more complex setups

---

### 🦐 **2. ESP8266 (Wi-Fi only)**
- **Pros:**
  - Even cheaper (~$1.50)
  - Mature, widely supported (NodeMCU, Arduino)
- **Cons:**
  - Only single-core
  - Less power efficient than ESP32, fewer GPIOs
- **Tip:** Can be a good choice if you're okay with Wi-Fi only and want the cheapest possible option

💡 Best for: **Low-budget simple Wi-Fi RGB lights**

---

### 🌱 **3. nRF52840 (Bluetooth LE)**
- **Pros:**
  - Ultra low power (great for battery life)
  - Bluetooth 5 / BLE Mesh support
  - Decent processing power
  - Available on affordable dev boards (e.g. Adafruit Feather, Dongle clones ~$5-10)
- **Cons:**
  - No Wi-Fi
  - BLE is harder to set up for instant app control unless you use specific apps or custom firmware
- **Bonus:** Can be used with **Zigbee / Thread** for mesh networks

💡 Best for: **Battery efficiency, BLE control, or future-proof smart home integration**

---

### 🏠 **4. Zigbee-capable MCUs (e.g., CC2530, EFR32, etc.)**
- **Pros:**
  - Ideal for smart home (compatible with Zigbee hubs like Home Assistant, Zigbee2MQTT)
  - Low power consumption
- **Cons:**
  - Development can be a bit trickier
  - Not as beginner-friendly or versatile outside smart home setups

💡 Best for: **Smart home integration with Zigbee (e.g., via Zigbee2MQTT)**

---
		
### 🐜 **5. Silicon Labs EFR32xG22 / EFR32BG22 (BLE 5.2)**
- **Wireless:** BLE 5.2, some variants support Zigbee/Thread
- **PWM:** Up to 8 PWM channels
- **Power:** Ultra-low power (down to µA range in sleep), designed for coin cells
- **Price:** ~$2.50 in quantity, dev boards from ~$5 (e.g., EFR32BG22 Thunderboard)
- **Pros:** Designed exactly for low-power BLE lighting and sensors
- **Cons:** Less Arduino-style ecosystem, uses SiLabs SDK / Simplicity Studio

💡 Best for **professional-grade BLE lighting control**, especially with mesh

---

### 🐝 **6. Telink TLSR8258 / TLSR8278 (BLE 5.0, Zigbee, RF4CE)**
- **Wireless:** BLE 5.0, Zigbee, 2.4 GHz proprietary
- **PWM:** Up to **9 PWM channels**
- **Power:** Super low power (sleep current ~0.5 µA)
- **Price:** **Very cheap** (< $1 in bulk; dev boards ~$3–5)
- **Dev Tools:** Telink BDTK SDK (not as beginner-friendly but usable)
- **Used in:** Tuya smart bulbs and Zigbee products

💡 Best for: **Zigbee-compatible RGB(W) lighting**, dirt-cheap designs

---

### 🧠 **7. Nordic nRF52810**
- **Wireless:** BLE 5.0
- **PWM:** 3 or 4 PWM (but can stretch with software)
- **Power:** Very low (~0.3 µA sleep, few mA active)
- **Price:** ~$1.50–2
- **Dev Tools:** Same as nRF52 family (good support)
- **Cons:** Fewer peripherals vs nRF52840

💡 Worth considering if 3–4 PWM channels are enough and you're okay with BLE

---

### 🦾 **8. Realtek RTL8762 Series**
- **Wireless:** BLE 5.0
- **PWM:** Up to **6–8 PWM** (varies by variant)
- **Power:** Designed for wearables, low current
- **Price:** Very cheap, used in many Chinese BLE products (e.g., earbuds, smartwatches)
- **Tools:** Realtek SDKs are hard to get, but there's some community effort

💡 Good option if you’re willing to reverse-engineer / use Chinese SDKs

---

### 📡 **9. Beken BK3432 / BK7231N**
- **Wireless:** BLE or Wi-Fi depending on variant
- **PWM:** 6–8 channels
- **Power:** Moderate-low
- **Used in:** Tuya smart devices, widely used in cheap Wi-Fi bulbs and BLE switches
- **Dev Tools:** Tuya SDK, or OpenBeken (community open firmware)
- **Price:** < $1 in modules

💡 **Great for hacking Tuya bulbs or building with OpenBeken**  
(Lots of Tuya modules use this + analog RGB LEDs)

---

### Bonus: STM32 + External RF
If you're open to **separating MCU and RF**, a low-end **STM32L0 or STM32G0** MCU + **RFM69 / RFM75 / NRF24L01+** module is still ultra-cheap and very flexible.

- STM32L031: low power + up to 10 PWM
- RF modules: <$1
- More setup work, but max flexibility and power efficiency

---

### Summary Table:

| SoC              | Wireless      | PWM Channels | Price (est.) | Notes |
|------------------|----------------|--------------|---------------|-------|
| **EFR32BG22**     | BLE 5.2        | 8            | ~$2.50        | Low power, strong SDK |
| **TLSR8278**      | BLE, Zigbee    | **9**        | <$1           | Cheap Tuya-style, but dev is tricky |
| **nRF52810**      | BLE 5.0        | 3–4          | ~$1.50        | Nordic quality, fewer PWMs |
| **RTL8762**       | BLE 5.0        | 6–8          | <$2           | Used in BLE gadgets |
| **BK7231N**       | Wi-Fi (some BLE)| 6–8         | <$1           | Tuya ecosystem, OpenBeken |
| **STM32L0 + RF**  | Custom (RF chip)| 6–10         | ~$1.50 total  | Ultra flexible combo |

## Battery
- [Lithium](https://amzn.eu/d/jimF2Nm)
- Alkaline Batteries?
- [Battery Discharge Curves](https://www.akkuline.de/test)

### 🔋 Battery life tip:
If you use **WS2812B (NeoPixels)** or similar, they draw significant power—even when idle. You might want to use **APA102 (DotStar)** or **analog RGB LEDs** with MOSFETs for better control and idle power saving.

## LED
- [RGB](https://www.ebay.de/itm/266288183437?mkcid=16&mkevt=1&mkrid=707-127634-2357-0&ssspo=2-Vdjos5RUS&sssrc=4429486&ssuid=EOFrFZG7Tz2&var=566114011470&widget_ver=artemis&media=MORE)

# Mechanics
- [Kanister](https://dipoxy.de/un-gefahrgut-kanister_2?gad_source=1&gbraid=0AAAAADrFZCmNNekIsefyHygD8Hkp3Zj90&gclid=Cj0KCQjwm7q-BhDRARIsACD6-fVCGfnqgCzTa02mOZyw8vGQtas-zCgIu3bxF9D_118SFL5lNDxCtB8aAorjEALw_wcB)


# Firmware

# Software
