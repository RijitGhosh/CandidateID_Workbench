# Embedded system design (host + microcontroller + FPGA + JTAG adapter)

**Everything in the application's Hardware page is SIMULATED.** No physical board or adapter was connected, powered or tested for this project.
Status tags: **[DOC]** = stated in a vendor document that was read for this project (source in `docs/embedded_research_notes.md`);
**[ASSUMED]** = a design decision or assumption, not from a datasheet; **[UNVERIFIED]** = could not be confirmed from the documents read. Do not build from [UNVERIFIED] or [ASSUMED] items without checking the vendor PDFs.

## 1. Selected parts
| Role | Part | Why |
|---|---|---|
| Microcontroller board | ST NUCLEO-F446RE (STM32F446RET6), user manual UM1724 | Public manual, onboard ST-LINK/V2-1 with USB virtual COM port |
| FPGA board | Digilent Arty A7-35T (Artix-7 XC7A35T) | Public reference manual, onboard FT2232HQ USB-JTAG/UART |
| JTAG debug adapter | Digilent JTAG-HS3 | Public reference manual; Xilinx 2x7, 2 mm JTAG cable |

**Finding that shaped the design:** (a) the Nucleo's onboard ST-LINK supports **SWD only** [DOC], and the JTAG-HS3 documents JTAG for Xilinx devices only, with no SWD support documented, so the HS3 does **not** debug the Nucleo; (b) the Arty already has onboard USB-JTAG (J10), and its external JTAG port J8 is documented for a **JTAG-HS2**, not an HS3 [DOC]. Plugging an HS3 into J8 is therefore **not a documented configuration**; the verified FPGA programming path is J10, and the HS3 would need a documented adapter cable and a confirmed 2x7 pin map (not available from the documents read). A documented alternative from the same vendor is the JTAG-HS2.

## 2. Block diagram
```
                         +-------------------------------+
                         |  Windows 11 host: Aether      |
                         |  (software adapters today are |
                         |   mocks; real ones = future)  |
                         +---+---------+-----------+-----+
                USB (Mini-B/ |         | USB       | USB (Micro-AB)
                see board)   |         | Micro-B   |
                             |         | J10       |
        +--------------------v--+   +--v-----------+-----+     +------------------------+
        | NUCLEO-F446RE  CN1    |   | Arty A7-35T  J10   |     | JTAG-HS3 (optional,    |
        | ST-LINK/V2-1 (SWD)    |   | FT2232HQ:          |     | not on documented path |
        |  + VCP USART2 PA2/PA3 |   |  USB-JTAG + UART   |     | to Arty J8)            |
        |        STM32F446      |   |      XC7A35T       |     |  2x7 2 mm Xilinx cable |
        |   3.3 V I/O, IOREF 3V3|   |  3.3 V I/O         |     |  Vref 1.8-5 V in       |
        +----------+------------+   +----+---------------+     +-----------+------------+
                   | UART (assumed)      |  Pmod JA (3.3 V, 200 ohm series)  | JTAG (to a target
                   | 3.3 V <-> 3.3 V     |                                   |  Xilinx JTAG port)
                   +---------------------+                                   |
                       + common GND (required) +-----------------------------+
```
Boards exchange data over one UART pair plus ground: Nucleo USART TX -> Arty Pmod RX and Arty Pmod TX -> Nucleo USART RX **[ASSUMED]**. Specific Nucleo pins and Arty package pins for this link are not chosen here (take them from UM1724 and the Digilent master XDC).

## 3. Interface and voltage table
| Interface | Signal / pin | Level | Direction (driver) | Status |
|---|---|---|---|---|
| Nucleo I/O | STM32 I/O, IOREF | 3.3 V (not 5 V as on Arduino Uno V3) | - | [DOC] UM1724 sec 7.13, Table 11 |
| Nucleo supply | USB CN1 (U5V): 300 mA max, 100 mA with JP1 on; VIN 7-12 V (800 mA at 7 V, 450 mA for 7-9 V, 250 mA for 9-12 V); E5V 4.75-5.25 V 500 mA; external 3.3 V input 3-3.6 V (ST-LINK unpowered) | 5 V / 3.3 V | host or supply | [DOC] |
| Nucleo ST-LINK to host | USB CN1; ST-LINK/V2-1 debug plus virtual COM port on USART2 (PA2/PA3) | 3.3 V UART | ST-LINK <-> MCU | [DOC] |
| Nucleo debug header CN4 | 1 VDD_TARGET, 2 SWCLK, 3 GND, 4 SWDIO, 5 NRST, 6 SWO (reserved) | 3.3 V | SWCLK from ST-LINK, SWDIO bidirectional | [DOC] Table 5 |
| Nucleo morpho SWD | SWDIO CN7 pin 13, SWCLK CN7 pin 15 | 3.3 V | - | [DOC] |
| Nucleo JTAG pins on MCU | MCU supports SWD and JTAG (SWJ-DP); JTAG pins not on CN4; exact header rows unconfirmed | 3.3 V | - | [DOC] support; [UNVERIFIED] rows |
| Arty I/O | FPGA I/O, Pmods, shield header | 3.3 V; shield header abs max 3.75 V, recommended max 3.4 V; not 5 V-shield compatible | - | [DOC] Arty RM sec 11 |
| Arty supply | USB J10; 7-15 V DC jack J13 (>= 1 A, centre-positive; 36 W ideal); or J7 pin 8 (VIN) | - | automatic source selection | [DOC]; USB current rating [UNVERIFIED] |
| Arty USB | Micro USB J10, FT2232HQ: USB-JTAG + USB-UART; UART on FPGA pins A9/D10 | 3.3 V | - | [DOC] |
| Arty Pmod | 2x6 female 100-mil; VCC pins 6 and 12 (3.3 V), GND pins 5 and 11, 8 signals; JA/JD with 200 ohm series | 3.3 V | - | [DOC] |
| Arty external JTAG | Port J8, documented for a JTAG-HS2; pin map/pitch | - | - | [DOC] purpose; [UNVERIFIED] pin map |
| JTAG-HS3 host | Micro-AB USB 2.0; powered from USB | 5 V USB | - | [DOC] |
| JTAG-HS3 target | Xilinx 2x7, 2 mm; up to 30 Mbit/s | Vref 1.8-5 V, must match the target JTAG supply (VCCO_0) | see section 6 | [DOC]; pin map except pin 14 [UNVERIFIED] |

## 4. Power architecture
- Each board is powered independently: Nucleo from host USB (300 mA limit with JP1 off [DOC]; ample for the MCU alone, avoid powering large shields from it), Arty from a 12 V supply on J13 or from USB J10 [DOC]; the HS3 is powered from its USB cable [DOC].
- No board powers another one. Do not connect the 3.3 V rails of different boards together, and do not connect a battery to J7 together with J13 [DOC, Arty RM].
- The HS3's Vref input is a **sense/supply reference from the target** [DOC]; it must come from the target JTAG supply, not from the HS3 USB side.
- [ASSUMED] Use a powered USB hub or separate host ports so that three USB devices do not overload one hub.

## 5. Grounding plan
- All signal connections require a **common ground**: Nucleo GND (e.g. CN6 pins 6/7 [DOC]) to Arty GND (Pmod pins 5 or 11, J7 pin 7 [DOC]) to JTAG adapter ground on its cable [ASSUMED location of ground pins on the 2x7 cable; confirm in the AMD/Xilinx or Digilent pin table].
- Connect grounds before signals when hot-plugging [ASSUMED good practice]. Keep the ground wire short and alongside the UART lines.
- USB shields all return to the host chassis ground through the host; avoid additional ground loops through separate mains-earthed supplies without checking (a [ASSUMED] precaution).

## 6. JTAG / debug signal ownership
| Signal | Driven by | Notes | Status |
|---|---|---|---|
| TCK, TMS, TDI | JTAG-HS3 (24 mA tri-state buffers, 100 ohm series, Hi-Z when idle) | Level = Vref | [DOC] |
| TDO | Target (FPGA) | Standard IEEE 1149.1 convention; not stated on the HS3 page read | [ASSUMED] convention / [UNVERIFIED] on HS3 page |
| Vref | Target | 1.8-5 V; HS3 buffer supply | [DOC] |
| Pin 14 PS_SRST_B | HS3, open-drain | Target needs pull-up of at least 1.5 kohm; 100 ohm series; Zynq PS reset, unused on Artix-7 | [DOC] |
| TRST | Not provided / not documented | Treat as unavailable | [UNVERIFIED] |
| Arty on-board path | FT2232HQ on J10 owns the JTAG chain of the XC7A35T | Do not drive J8 at the same time as J10 [ASSUMED] | [DOC] existence |
| Nucleo | ST-LINK/V2-1 owns SWD (SWCLK, SWDIO, NRST) | One debugger at a time; remove both CN2 jumpers only to program an external target via CN4 | [DOC] |

## 7. Voltage-domain compatibility and level shifting
| Connection | Domains | Verdict |
|---|---|---|
| Nucleo UART <-> Arty Pmod | 3.3 V <-> 3.3 V | Direct connection is compatible; no level shifter needed [ASSUMED, based on both documented 3.3 V]. Arty Pmod JA/JD have 200 ohm series resistors [DOC]. |
| HS3 <-> Arty JTAG | Vref follows target VCCO_0 | Bank 0 VCCO value is [UNVERIFIED]; expected 3.3 V [ASSUMED], within the HS3's 1.8-5 V range. No shifter needed for the HS3 itself. |
| Any 5 V peripheral (Arduino shield, 5 V UART) <-> either board | 5 V <-> 3.3 V | **Level shifting required.** The Arty shield header is 3.3 V only (recommended max 3.4 V, abs max 3.75 V [DOC]); the STM32 header guidance is 3.3 V, although the datasheet lists 5 V-tolerant I/Os [DOC], which is not a licence to drive a 5 V push-pull source into the board. |
| 1.8 V devices | 1.8 V <-> 3.3 V | Level shifting required [ASSUMED] |

## 8. Host communication
- Nucleo: virtual COM port over CN1 (USART2 PA2/PA3) [DOC]; the host sees a COM port.
- Arty: FT2232HQ exposes a USB-UART (FPGA pins A9/D10) and JTAG [DOC]. Programming uses the vendor tool.
- HS3: USB 2.0 device used by Xilinx tooling.
- [ASSUMED] Future Aether adapters would speak a simple line-based protocol over each COM port, implement the same interface as the mocks (discover, capabilities, connect, disconnect, one telemetry message, timeout error), and keep JTAG/SWD control with the vendor tools; Aether would only read status.

## 9. Connectors
| Device | Connector | Notes |
|---|---|---|
| Nucleo | USB CN1 (Fig 3 labels Mini-B; cable type depends on board revision, check the board); Arduino V3 headers CN5/CN6/CN8/CN9; ST morpho CN7/CN10; ST-LINK header CN4 (6 pins) | [DOC]; header pitch [UNVERIFIED] (2.54 mm assumed) |
| Arty | Micro USB J10; power jack J13 (coaxial, 2.1 or 2.5 mm: sources disagree, check the jack); Pmod 2x6 100-mil; shield header J7; external JTAG J8 | [DOC] |
| HS3 | Micro-AB USB; 2x7 2 mm Xilinx header cable | [DOC] |

## 10. Device-specific pinout assumptions
- Nucleo USART2 PA2/PA3 is reserved for the ST-LINK virtual COM port (solder bridges SB13/SB14 on, SB62/SB63 off by default) [DOC]. For board-to-board UART use another USART and read its pins from UM1724 before wiring [ASSUMED].
- Arty UART to FT2232HQ uses FPGA pins A9 (TXD) and D10 (RXD) [DOC]; the board-to-board UART should use a Pmod pair and constraints from the Digilent master XDC [ASSUMED, not verified here].
- Nucleo CN4 pinout above is [DOC]; HS3 2x7 pinout is **not reproduced here** because it was not confirmed from the documents read.

## 11. Why a JTAG adapter is not a generic, interchangeable compute board
A JTAG adapter is a fixed-function protocol translator: its firmware maps USB commands to a specific set of signals (TCK/TMS/TDI/TDO, reference voltage, optionally reset) on a defined connector. It has no user-programmable application processor, no general I/O the user can reassign, and its output buffers are referenced to the **target's** supply (the HS3 runs its buffers from Vref). It is selected by what its software tools support (the HS3: Xilinx tooling; SWD/Arm targets are not documented for it), and it only acts when a target answers on the chain. A microcontroller or FPGA board, in contrast, runs user logic. Replacing the debug adapter with a compute board would remove the vendor protocol support, the voltage-referenced buffering and the tool integration, and it would not provide the debug access.

## 12. Simulated behaviour (what the mocks do)
`src/core/hardware/mock_devices.*`: each device supports discovery, capability report, connect, disconnect, one deterministic telemetry message and a **timeout fault**: `armTimeoutFault()` makes the next `readTelemetry()` return `Timeout` and puts the device in `Faulted`; `recover()` restores `Connected`. Tests: `tests/test_hardware.cpp`. Every telemetry message carries `simulated = true` and the banner "SIMULATED - no physical hardware involved".

## 13. Safe future integration plan and limitations
1. Add real adapters behind the same interface, **read-only first**: open the COM port, listen, parse one status line. No flashing, no JTAG/SWD commands from Aether.
2. Before first power-up verify with a multimeter: 3.3 V on the Nucleo IOREF and Arty 3V3, continuity of the common ground, and no 5 V on any signal.
3. Hot-plug procedure: power both boards, connect ground first, then the UART pair.
4. Keep timeouts and the recover path from the mock; treat any unplugged device as a normal condition, not a crash.
5. Limitations: UNVERIFIED items above; no hardware was available; USB bandwidth, ESD and EMC are not analysed; the pinout of the HS3 cable and Arty J8 must be checked before any cable is made.
