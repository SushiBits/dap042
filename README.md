# dap042

The CMSIS-DAP debug probe for the SUSHiBITS&reg; One Pro: an STM32F042 that
turns a USB port into an SWD and JTAG debugger for the board's main MCU.

It speaks CMSIS-DAP v2 over USB HID, so it works with anything that speaks the
protocol - OpenOCD, pyOCD, probe-rs, Keil - with no vendor driver.

```
USB (composite)                    target debug port
┌─────────────────────────┐        ┌──────────────────────────┐
│ STM32F042               │        │ SWCLK/TCK   PA5          │
│  free-dap  (protocol)   │◄──────►│ SWDIO/TMS   PA4          │
│  libusb_stm32  (USB)    │        │ TDI         PA7          │
└─────────────────────────┘        │ TDO         PA6          │
                                   │ nRESET      PB1          │
                                   └──────────────────────────┘
```

## Status

Working:

- USB composite device, CMSIS-DAP v2 HID transport (64-byte reports)
- SWD and JTAG, 1 kHz to 3 MHz
- Flash programming and verification through OpenOCD

`main` clocks USB from the PLL. This branch (`usb-hsi48-sof-trim`) runs USB
from HSI48 with CRS trimming against the host's SOF frames instead, leaving the
PLL to SYSCLK and everything else. It has not been run on hardware yet; see
[USB clock and SOF trimming](#usb-clock-and-sof-trimming).

Not implemented yet:

- CDC-ACM virtual serial port. The function enumerates and answers the class
  requests, but no UART backend is attached.
- Recovery from a wedged command. A DAP command that never returns leaves the
  probe enumerating but mute, and the only way out is a replug. The protocol
  layer no longer trusts host-supplied timing parameters (see
  [Byte-level protocol notes](#byte-level-protocol-notes)), but a watchdog is
  deferred until the CDC part lands - the idle loop sleeps so deeply that a
  naive IWDG would reset a healthy suspended probe, so this needs a proper wake
  source rather than a kick in the main loop.
- `DAP_TransferBlock` is limited to the 64-byte report (15 words read, 14
  write). Batching across packets is not implemented.
- `DAP_JTAG_IDCODE` returns garbage on multi-TAP chains; hosts use
  `DAP_JTAG_Sequence` instead.

## Hardware

| | |
|---|---|
| MCU | STM32F042F4 (16 KiB flash) for production, STM32F042F6 (32 KiB) for development |
| Clock | 12 MHz crystal, PLL x4 = 48 MHz SYSCLK; USB from HSI48, CRS-trimmed to the host's SOF frames |
| USB VID:PID | `0x0002:0xDA42` (the `0x0002` vendor ID is legacy, not USB-IF assigned) |
| USB class | composite: HID (CMSIS-DAP v1) + CDC-ACM |
| Firmware version | `2.0.0` |

The build links against the **F042F4 memory map by default** so that running
out of the 16 KiB production part is a link error rather than something you
discover on a shipped unit. See [Building](#building) for the development
override.

### Use a crystal, not a ceramic resonator

The board uses a 12 MHz crystal for HSE. On `main`, that crystal *is* the USB
clock, and USB full-speed requires the 48 MHz peripheral clock to stay within
**±0.25 %** - a budget that covers the part's initial tolerance *and* its drift
over temperature and ageing. A ceramic resonator does not have the margin: the
cheap tight-tolerance parts are typically ±0.5 % over temperature before ageing
is counted.

A resonator that misses the budget does not fail cleanly. The device still
enumerates, the host still sends frames, and every packet is rejected with a
CRC error - which reads as a dead or half-broken probe rather than a clock
problem, and costs hours to diagnose.

This branch moves USB off the HSE path, so that argument no longer binds: see
[USB clock and SOF trimming](#usb-clock-and-sof-trimming) below. Keep the
crystal anyway - the cost difference is small, and it is what lets the
PLL-clocked variant be one line away.

Earlier revisions of this board used a Murata CERALOCK resonator; current
builds use a 12 MHz crystal. Keep it that way.

### USB clock and SOF trimming

`main` clocks USB from the PLL, which means the crystal's absolute accuracy is
also the USB clock's accuracy. This branch separates the two: `SystemInit`
enables HSI48, points `RCC_CFGR3.USBSW` at it, and starts the CRS with
`AUTOTRIMEN | CEN` so the hardware trims HSI48 against the 1 kHz SOF frames the
host is already sending. HSE and the PLL still clock SYSCLK and everything
else.

`CRS_CFGR` resets to `SYNCSRC = USB SOF`, `RELOAD = 47999` (one 1 kHz period
measured at 48 MHz) and `FELIM = 0x22`, which is already the right measurement
window, so the code only starts the counter rather than reprogramming it.

What this buys and what it costs:

- **The pre-enumeration window is out of spec, deliberately.** Until the first
  SOF arrives, HSI48 runs on its factory trim, which is looser than the
  ±0.25 % budget. Only then does the CRS pull it in. The device has to get
  through the reset and address handshake on the untrimmed oscillator, and
  whether a given host tolerates that is exactly what this branch is for.
- **Trimming can stay on permanently.** While the bus is suspended no SOF
  arrives, so the CRS simply holds its last correction; nothing has to stop or
  restart it around enumeration, suspend or resume.
- **The clock domains are independent.** The PLL can be reconfigured for the
  CDC work, or SYSCLK changed, without disturbing a live USB link.
- **Timing calibration is unaffected.** `SystemCoreClock` is still 48 MHz, so
  SysTick, `delay_ms()` and the measured `DAP_CONFIG_DELAY_CONSTANT` /
  `DAP_CONFIG_FAST_CLOCK` values carry over unchanged.

## Building

Requires `arm-none-eabi-gcc`, `make`, and Python 3 (the IRQ handler headers are
generated by `contrib/irqn`).

```sh
git submodule update --init --recursive
make
```

Outputs `dap042.elf` in the repository root; object files and dependency files
live under `.build/`.

| target | purpose |
|---|---|
| `make` | build the ELF |
| `make hex` / `make bin` | Intel HEX / raw binary for production programming |
| `make size` | text/data/bss totals plus a per-section breakdown |
| `make clean` | remove targets and `.build/` |
| `make distclean` | also remove the generated `*_handlers.h` |

Build for the 32 KiB development part:

```sh
make MEMORY_LD=stm32f042x6
```

### Size budget

The current image is about 13.0 KiB of the production part's 16 KiB. If space
gets tight, in order of value:

1. `make OPTFLAGS="-Os -g3"` - roughly 800 bytes, no behaviour change.
2. `-flto` (same as free-dap's own makefiles) - a few hundred more.
3. Drop JTAG with `//#define DAP_CONFIG_ENABLE_JTAG` - about 1.5 KiB of text.

## Using it with OpenOCD

The vendor ID is not in OpenOCD's probe table, so select the probe explicitly:

```sh
openocd -f interface/cmsis-dap.cfg \
        -c "cmsis-dap vid_pid 0x0002 0xda42" \
        -c "transport select swd" -c "adapter speed 1000" \
        -f target/stm32f1x.cfg
```

For JTAG, swap the transport. `target/stm32f1x.cfg` already declares both TAPs
the F103 exposes (JTAG-DP plus boundary scan), so **do not add your own
`jtag newtap`** unless the chain has devices the target file does not know
about - a phantom tap shifts the whole chain and every capture reads back as
all ones:

```sh
openocd -f interface/cmsis-dap.cfg \
        -c "cmsis-dap vid_pid 0x0002 0xda42" \
        -c "transport select jtag" -c "adapter speed 1000" \
        -f target/stm32f1x.cfg
```

## Architecture

Three layers, and it matters which one owns a bug:

| layer | files | responsibility |
|---|---|---|
| protocol | `src/dap.c`, `include/dap.h` | CMSIS-DAP command dispatch, SWJ timing. Vendored from [free-dap](https://github.com/ataradov/free-dap). |
| USB | `contrib/libusb_stm32` | enumeration, HID and CDC endpoints. |
| board glue | `src/main.c`, `src/usbmain.c`, `src/usbhid.c`, `include/dap_config.h` | pin and clock setup, the request/response hand-off. |

**The protocol never runs in interrupt context.** `USB_IRQHandler` only moves
bytes through `usbd_poll()`; the main loop calls `dap_process_request()`.
Bit-banging SWD with `DAP_CONFIG_DELAY` in the path takes milliseconds, which
would starve USB if it ran inside the ISR. Buffers are handed over with
`volatile` flags and PRIMASK guards, one request in flight at a time: the
receive endpoint stays NAK until the previous response is queued, and the host
retries.

`DAP_CONFIG_*` in `include/dap_config.h` is the whole hardware seam - pins,
initialisation, and the timing constants below.

## Timing calibration

Two constants decide every clock the probe generates. Both were **measured on
hardware** with a logic analyser on TCK, at the default `-Og` build:

```c
#define DAP_CONFIG_DELAY_CONSTANT  6000       // 1 kHz test output on SWCLK
#define DAP_CONFIG_FAST_CLOCK      1800000    // above this, no delays are added
```

They are board- and compiler-specific: any change to the optimisation level,
the delay loop, or `SystemCoreClock` invalidates them, and the symptom is a
probe that reports a clock it does not produce. Re-measure after such a change
rather than scaling the old numbers.

free-dap computes the per-bit delay as
`(DAP_CONFIG_DELAY_CONSTANT * 1000) / frequency`, so `DELAY_CONSTANT` is defined
as the delay value that produces **1 kHz** on SWCLK, and `FAST_CLOCK` is the
frequency that `dap_clock_test(1)` actually achieves.

Calibrate with a scope or logic analyser on TCK:

1. Run `dap_clock_test(DAP_CONFIG_DELAY_CONSTANT)` and measure. Iterate with
   `C_new = C_old * f_measured / 1000` until it reads 1.000 kHz.
2. Run `dap_clock_test(1)` for the real ceiling, then set `FAST_CLOCK` to it
   with 10-20 % margin. Err low: switching to the no-delay path at a clock the
   loop cannot sustain turns into SWD ACK failures that look like a target
   problem.
3. Sweep `DAP_SWJ_Clock` from 100 kHz to 24 MHz and check the result is
   monotonic with no step at the `FAST_CLOCK` boundary.

Record the measured values and the date next to the constants. Do not copy
numbers from another platform - they are only valid for a given CPU clock,
flash wait-state configuration and GPIO implementation.

## Byte-level protocol notes

`include/dap_config.h` sets `DAP_CONFIG_PACKET_SIZE` to the HID report size
(64 bytes) rather than the 512 that free-dap defaults to for bulk transports.
`DAP_CONFIG_PACKET_COUNT` is 1, matching the single outstanding transfer the
glue implements.

Five behaviours here differ from upstream free-dap and are worth knowing about
when merging:

- `DAP_SWJ_Pins` treats `Wait` as microseconds, per the specification. Upstream
  multiplies by 1000.
- `DAP_SWJ_Clock` rejects 0 Hz, and `DAP_SWJ_Sequence` reads a zero length as
  256 bits. Upstream divides by zero and clocks nothing respectively.
- `DAP_TransferBlock` rejects a count that does not fit the report instead of
  returning a response whose count disagrees with the data.
- `DAP_{JTAG,SWD}_Sequence` counts the results before it drives a clock, and
  answers `DAP_ERROR` if they will not fit. Upstream replies `DAP_OK` and
  silently drops the trailing ones, having already moved the target.
- An overrun detected by `dap_is_buf_error()` is replaced, in the glue, with the
  shortest response that is valid for that command. Upstream would send
  whatever partially-filled buffer it had, which the host parses as data.
  `dap_build_error_response()` holds the per-command table, because the byte
  after the command ID means different things to different commands.

## License

BSD 3-Clause. See [LICENSE.md](LICENSE.md).

`src/dap.c`, `include/dap.h` and the original `include/dap_config.h` are
BSD 3-Clause, copyright Alex Taradov (free-dap). `contrib/libusb_stm32` is
Apache 2.0, copyright Dmitry Filimonchuk. The CMSIS device headers under
`contrib/platform` are STMicroelectronics'.

