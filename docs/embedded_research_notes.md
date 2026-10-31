# Embedded research notes (source-cited)

Method note: values were obtained by fetching vendor documents with a summarising fetch tool (page text, not raw PDF bytes), so figures should be re-checked against the PDF before release. Direct PDF download (curl) was blocked by the proxy. Anything not seen in fetched text is UNVERIFIED. Today: 2026-10-31.

Source keys:
- [UM1724] https://www.st.com/resource/en/user_manual/DM00105823-.pdf (UM1724 Rev 17, STM32 Nucleo-64 boards MB1136)
- [DS10693] https://www.st.com/resource/en/datasheet/stm32f446re.pdf (STM32F446xC/E datasheet, Rev 11)
- [ARTY-RM] https://assets.testequity.com/te1/Documents/pdf/digilent/Digilent_Arty_A7_Reference_Manual.pdf (Digilent Arty A7 Reference Manual; reseller-hosted copy of the Digilent PDF. digilent.com manual URLs were not fetchable in this session.)
- [ARTY-WIKI] https://reference.digilentinc.com/programmable-logic/arty-a7/start
- [HS3-RM] https://reference.digilentinc.com/jtag_hs3/refmanual (Digilent JTAG-HS3 Reference Manual)
- [HS2-RM] https://digilent.com/reference/programmers/jtag-hs2/reference-manual

## 1. ST NUCLEO-F446RE (STM32F446RET6, UM1724)

| Item | Value | Source URL | Status |
|---|---|---|---|
| Board/user manual | UM1724 Rev 17 covers STM32 Nucleo-64 (MB1136) | [UM1724] | DOCUMENTED |
| I/O logic level | STM32 I/Os are 3.3 V compatible, not 5 V as on Arduino Uno V3 (Sec 7.13, p.38) | [UM1724] | DOCUMENTED |
| IOREF | 3.3 V (Table 11, p.38) | [UM1724] | DOCUMENTED |
| MCU VDD range | 1.7 V to 3.6 V per feature list (1.7 V needs supervisor); Table 2 says 1.8 to 3.6 V | [DS10693] | DOCUMENTED |
| 5 V tolerance | Datasheet: "Up to 112 5 V-tolerant I/Os" (board header guidance still says 3.3 V) | [DS10693] | DOCUMENTED |
| Power: USB (CN1, U5V) | 300 mA max with JP1 OFF; 100 mA with JP1 ON (Sec 7.5.1, Table 6) | [UM1724] | DOCUMENTED |
| Power: VIN (CN6 pin 8 / CN7 pin 24) | 7 to 12 V; 800 mA at 7 V, 450 mA for 7<Vin<=9 V, 250 mA for 9<Vin<=12 V (Table 7, p.22) | [UM1724] | DOCUMENTED |
| Power: E5V (CN7 pin 6) | 4.75 to 5.25 V, 500 mA max | [UM1724] | DOCUMENTED |
| Power: +3.3 V input (CN6 pin 4, CN7 pins 12, 16) | 3 to 3.6 V; ST-LINK unpowered in this mode (SB2/SB12 OFF) (Sec 7.5.3, Table 9) | [UM1724] | DOCUMENTED |
| Power outputs | +5 V (CN6 pin 5, CN7 pin 18); +3.3 V regulator supplies up to 500 mA (Sec 7.5.4) | [UM1724] | DOCUMENTED |
| USB connector | CN1; Fig 3 labels Mini-B; Sec 3.1 says cable type (Micro-B/Mini-B/Type-C) depends on board reference | [UM1724] | DOCUMENTED (check physical board revision) |
| USB interfaces | ST-LINK/V2-1 debug/programming plus Virtual COM port (Sec 7.4) | [UM1724] | DOCUMENTED |
| VCP wiring | USART2 on PA2/PA3; SB13, SB14 ON and SB62, SB63 OFF (default) (Sec 7.10; Table 10) | [UM1724] | DOCUMENTED |
| Debug protocol of onboard ST-LINK | "supports only SWD for STM32 devices" (Sec 7.4, p.17) | [UM1724] | DOCUMENTED |
| ST-LINK CN4 header (SWD) | Pin 1 VDD_TARGET, 2 SWCLK, 3 GND, 4 SWDIO, 5 NRST, 6 SWO (reserved) (Table 5, p.20) | [UM1724] | DOCUMENTED |
| ST-LINK for external target | Remove both CN2 jumpers: ST-LINK drives external CN4 (SWD supported). CN2 ON: programs on-board MCU (Table 4, Sec 7.4.5). SB12 must be OFF if CN4 pin 5 used externally | [UM1724] | DOCUMENTED |
| SWD on morpho | SWDIO CN7 pin 13, SWCLK CN7 pin 15 (Sec 7.3, p.17) | [UM1724] | DOCUMENTED |
| Arduino Uno V3 headers | CN5, CN6, CN8, CN9, female Arduino-compatible | [UM1724] | DOCUMENTED |
| ST morpho headers | CN7 and CN10 (Sec 7.12/7.14) | [UM1724] | DOCUMENTED |
| Header pitch (morpho / Arduino) | Not stated in fetched text | [UM1724] | UNVERIFIED |
| GND pins | CN6 pins 6, 7 and CN5 pin 7 are GND (Table 11); further GND on morpho (Figs pp.29-37) | [UM1724] | DOCUMENTED (partial) |
| AVDD / VREF | CN5 pin 8 (AREF) is AVDD; SB57 ON ties VDDA/VREF+ to VDD; remove SB57 for external VREF+ | [UM1724] | DOCUMENTED |
| MCU supports SWD and JTAG | Datasheet feature list: "SWD and JTAG interfaces"; SWJ-DP (Sec 3.40) | [DS10693] | DOCUMENTED |
| JTAG pins (PA13/PA14/PA15/PB3/PB4) alternate-function rows and LQFP64 pin numbers | Not present in fetched text | [DS10693] | UNVERIFIED |
| Whether PA15/PB3/PB4 (JTDI/JTDO/NJTRST) are broken out on header and shared with Arduino/morpho signals | Not confirmed (PB3 is SWO when SB15 ON) | [UM1724] | UNVERIFIED |
| Full JTAG (TCK/TMS/TDI/TDO/NJTRST) via CN4 | CN4 carries SWD only; no JTAG pins on CN4 | [UM1724] | DOCUMENTED |

Interface ownership: on the Nucleo the onboard ST-LINK/V2-1 owns SWD to the F446 and the VCP on USART2 (PA2/PA3). The JTAG-HS3 is JTAG-only for Xilinx devices and has no documented SWD support, so it is NOT directly applicable to the Nucleo.

## 2. Digilent Arty A7-35T (Artix-7 XC7A35T)

| Item | Value | Source URL | Status |
|---|---|---|---|
| FPGA family | Xilinx Artix-7; programmable over JTAG and Quad-SPI flash | [ARTY-WIKI] | DOCUMENTED |
| Power inputs | USB-JTAG port J10; 7-15 V DC at power jack J13; or J7 pin 8 (VIN) | [ARTY-RM] Sec 3 | DOCUMENTED |
| Power jack | Coaxial, center-positive, 2.1 mm (or 2.5 mm) ID, 7-15 V, at least 1 A; ideal 36 W (12 V, 3 A). Wiki says 2.5 mm coaxial | [ARTY-RM], [ARTY-WIKI] | DOCUMENTED (2.1 vs 2.5 mm wording inconsistent in sources) |
| Source selection | Automatic via USB load switch IC13 and MOSFET Q8; no select jumper | [ARTY-RM] | DOCUMENTED |
| Battery input | + to J7 pin 8 (VIN), - to J7 pin 7 (GND); do not connect with J13 supply | [ARTY-RM] | DOCUMENTED |
| USB current rating | Not given ("enough power for the vast majority of designs") | [ARTY-RM] | UNVERIFIED (no number) |
| 3.3 V rail | Supplies FPGA I/O, clocks, flash, Pmods, etc.; IC11 (DA9062), max 2.0 A | [ARTY-RM] Table 3.1 | DOCUMENTED |
| Bank 0 VCCO value | Not stated in fetched text | [ARTY-RM] | UNVERIFIED |
| USB connector / bridge | Micro USB J10; FTDI FT2232HQ provides USB-JTAG and USB-UART | [ARTY-RM] Sec 4.1, 8 | DOCUMENTED |
| UART pins | FPGA A9 and D10 (TXD/RXD) | [ARTY-RM] Sec 8 | DOCUMENTED |
| Onboard JTAG | Digilent USB-JTAG circuit on J10 controlled by FT2232HQ | [ARTY-RM] Sec 4.1 | DOCUMENTED |
| External JTAG port | J8, for "an external JTAG programmer, such as the Digilent JTAG-HS2" | [ARTY-RM] Sec 4.1 | DOCUMENTED |
| J8 pin count / pitch / pin-signal map | Not in fetched text. [HS2-RM] describes HS2 as using a 6-pin, 100-mil Digilent header, which suggests J8 is that style | [ARTY-RM], [HS2-RM] | UNVERIFIED |
| JTAG-HS3 mentioned in Arty manual | No | [ARTY-RM] | DOCUMENTED (absence in fetched text) |
| Config mode jumper | JP1 selects Quad-SPI boot; JP2 (CK_RST) ties FTDI to MicroBlaze reset | [ARTY-RM] | DOCUMENTED |
| Quad-SPI flash | Spansion S25FL128SAGMF100, 128 Mb; up to 50 MHz programming clock | [ARTY-RM] Sec 5.2 | DOCUMENTED |
| Pmod | 2x6 right-angle female, 100-mil; 12 pins; VCC pins 6, 12 (3.3 V); GND pins 5, 11; 8 signals; VCC/GND up to 1 A per doc | [ARTY-RM] Sec 10 | DOCUMENTED |
| Pmod series protection | JA, JD: 200 ohm; JB, JC: 0 ohm shunts, differential pairs | [ARTY-RM] | DOCUMENTED |
| Shield header J7 logic level | 3.3 V, IOREF = 3.3 V; abs max powered digital pin 3.75 V; recommended max 3.4 V; not 5 V-shield compatible; 49 GPIO each 200 ohm | [ARTY-RM] Sec 11 | DOCUMENTED |
| Shield analog | A0-A5 single-ended 0-3.3 V scaled; A6-A11 differential 0-1 V | [ARTY-RM] | DOCUMENTED |
| Shield power pins | 3V3, 5V0, VIN, GND, RST; J7 pin 7 = GND, pin 8 = VIN | [ARTY-RM] | DOCUMENTED |
| Reset | RESET button tied to J7 RST and via JP2 to FTDI; PROG button clears configuration | [ARTY-RM] | DOCUMENTED |
| 35T bitstream size | 17,536,096 bits | [ARTY-RM] | DOCUMENTED |

## 3. Digilent JTAG-HS3

| Item | Value | Source URL | Status |
|---|---|---|---|
| Purpose | High-speed programming/debug for Xilinx FPGAs and SoCs; Zynq-7000 and Zynq UltraScale+ named | [HS3-RM] | DOCUMENTED |
| Target connector | Standard Xilinx 2x7, 2 mm JTAG header (cable) | [HS3-RM] | DOCUMENTED |
| Host interface | Micro-AB USB2 connector on module; standard Type-A to Micro-USB cable; powered from USB | [HS3-RM] | DOCUMENTED |
| Max JTAG speed | Up to 30 Mbit/s, user adjustable | [HS3-RM] | DOCUMENTED |
| Vref range | "any voltage between 1.8V and 5V"; must be tied to same supply as FPGA JTAG port (VCCO_0) | [HS3-RM] | DOCUMENTED |
| Signals driven by HS3 | TMS, TDI, TCK via 24 mA three-state buffers (max 50 mA); 100 ohm series resistors; Hi-Z except when driving | [HS3-RM] | DOCUMENTED |
| Vref direction | Supply/reference input to HS3 from target (powers HS3 buffers); target-side supply | [HS3-RM] (text: Vref pin supplies the buffers) | DOCUMENTED |
| Pin 14 | PS_SRST_B (Zynq PS reset), driven by open-drain buffer on HS3; target pull-up at least 1.5 kohm; 100 ohm series | [HS3-RM] | DOCUMENTED |
| TDO | Name present; direction (target output, HS3 input) is implied but not stated in HS3 page. HS2 page states HS2 receives TDO | [HS3-RM], [HS2-RM] | UNVERIFIED for HS3 (inferred from HS2) |
| TRST | Not mentioned | [HS3-RM] | UNVERIFIED (treat as not provided) |
| 2x7 pin-number to signal map (except pin 14) | Not in fetched text | [HS3-RM] | UNVERIFIED |
| FTDI chip inside | Not mentioned | [HS3-RM] | UNVERIFIED |
| SWD / Arm support | Not mentioned; page describes JTAG and Xilinx only | [HS3-RM] | UNVERIFIED (no SWD support documented) |
| Arty A7 compatibility | Arty manual names JTAG-HS2 on J8, not HS3 | [ARTY-RM] | DOCUMENTED |

## 4. Parts-selection findings

1. Nucleo-F446RE: well documented. Its onboard ST-LINK is SWD-only, so JTAG-HS3 does not apply to it. Use ST-LINK (CN1 USB, or CN4 for an external SWD target).
2. Arty A7-35T: has onboard FT2232HQ USB-JTAG on J10, so the HS3 is not needed. The board's external port J8 is documented for a JTAG-HS2 style programmer; an HS3 (2x7, 2 mm) plugging into J8 is NOT documented and likely needs an adapter. Pin map of J8 unverified.
3. JTAG-HS3 vs Arty: documented alternative from the same vendor is the JTAG-HS2 (named in Arty manual for J8). Alternatively use the Arty's onboard USB-JTAG and treat the HS3 as a generic Xilinx 2x7 adapter in the design doc, citing its limits.
4. Get the exact 2x7 pinout from the Xilinx/AMD JTAG header documentation (not fetched here) before publishing any pin table.

## 5. Design assumptions (NOT from datasheets)

- The design doc will treat the Nucleo as a 3.3 V logic domain and the Arty as a 3.3 V logic domain, and require level shifting for any 5 V device.
- Host-side tool ownership: ST-LINK owns SWD on the Nucleo; FT2232HQ (or HS2/HS3) owns JTAG on the Arty.
- HS3 Vref is assumed to be wired to the target's JTAG supply (3.3 V on Arty) when used via an adapter.
- Common ground between boards and adapters is assumed required for any shared signalling.
- Header pitch for Nucleo morpho/Arduino is assumed standard 2.54 mm (UNVERIFIED here).
- Standard Xilinx 2x7 pin ordering is assumed only after it is confirmed from an official AMD/Xilinx or Digilent document.
