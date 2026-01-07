# STM32L4 (Cortex-M4) FOTA Documentation

**Target MCU**: STM32L476xx (ARM Cortex-M4 with FPU) – 1 MB Flash, 128 KB RAM.  
Easily adaptable to other STM32L4 devices by adjusting linker scripts and flash definitions.

**Key Features**
- AES-CBC-MAC based firmware signing and verification
- Reliable packetized firmware transfer over UART
- Comprehensive bootloader command interface
- CRC32 integrity checks on packets and full image
- Finite State Machine (FSM) for robust, interruptible updates
- Shared flash API and utilities between bootloader and application

---

## Table of Contents

- [Memory Layout](#memory-layout)
- [Bootloader Architecture](#bootloader-architecture)
- [Firmware Update Flow](#firmware-update-flow)
- [AES-CBC-MAC Verification](#aes-cbc-mac-verification)
- [Bootloader Commands](#bootloader-commands)
- [Python Host Utilities](#python-host-utilities)

---

## Memory Layout

The 1 MB flash is strictly partitioned to ensure bootloader safety and reliable updates.

| Region        | Start Address | Size    | Pages  | Description                                      |
| ------------- | ------------- | ------- | ------ | ------------------------------------------------ |
| Bootloader    | 0x08000000    | 64 KB   | 0–31   | Bootloader code and data (never overwritten)     |
| FOTA Metadata | 0x08010000    | 2 KB    | 32     | Update status, vector table copy, CRC, signature |
| Application   | 0x08010800    | ~942 KB | 33–511 | Active application firmware (updatable slot)     |

> Note: Exact sizes and addresses are defined in:
> - `bootloader/bootloader.ld`
> - `app/app.ld`
> - `common/memory_map.ld` (shared)

The bootloader redirects the vector table to the metadata region during updates and restores it to the application start on successful boot.

---

## Bootloader Architecture

The bootloader is located in `cortex-m4/bootloader/`.

### Responsibilities
- Hardware initialization (GPIO, UART, CRC, TIM)
- Validation of existing application (AES signature + CRC)
- Entry into update mode if invalid or forced
- Secure reception and installation of new firmware
- Safe jump to validated application

### Core Components (`Core/Src` & `Core/Inc`)
- `bootloader.c/h` – Entry point, validation, jump logic
- `bootloader_fsm.c/h` – FSM controlling update states
- `bootloader_cmds.c/h` – Command parsing and execution
- `aes.c/h` – AES-128 implementation for CBC-MAC
- `packet_controller.c/h` – Packet framing, sequencing, CRC
- `bl_packet_responder.c/h` – ACK/NACK handling
- `comms.c/h` – UART RX/TX with ring buffer

### Shared Components (`../common/`)
- `fota_api.c/h` – Flash erase/write, jump functions
- `is_ringbuffer.c/h` – Interrupt-safe UART buffer
- `msg_printer.c/h` – Debug printing
- `versions.h` – Version definitions

---

## Firmware Update Flow

1. Device powers on → Bootloader runs
2. Check metadata for valid application signature/CRC
3. If valid → Jump to application
4. If invalid or update forced → Enter command mode
5. Host connects via `serial_monitor.py`
6. Sequence:
   - Sync → Verify Device ID → Erase Flash → Send Size → Send Packets → Verify → Jump
7. On success: Update metadata and reset into new app
8. On failure: Remain in bootloader

The FSM ensures partial updates can be resumed or aborted safely.

---

## AES-CBC-MAC Verification

Firmware authenticity uses **AES-128 CBC-MAC** (not encryption).

### Signing (Host Side – `fw-signer.py`)
- Pad binary to 16-byte boundary
- Compute CBC-MAC using fixed shared key
- Append 16-byte MAC tag to binary

### Verification (Bootloader)
- Receive full binary + tag
- Recompute CBC-MAC over firmware data
- Compare against received tag
- Only proceed if match

This provides strong authentication with minimal overhead.

---

## Bootloader Commands

| Command                | Description                         | Parameters      |
| ---------------------- | ----------------------------------- | --------------- |
| Sync                   | Handshake / connection establish    | None            |
| Get Bootloader Version | Read bootloader version             | None            |
| Get App Version        | Read current app version (if valid) | None            |
| Get Chip ID            | Read unique device ID               | None            |
| Get RDP Level          | Read readout protection level       | None            |
| Verify Device ID       | Confirm target device               | Expected ID     |
| Erase Flash            | Erase application region            | None            |
| Send Firmware Size     | Declare incoming binary size        | Size (bytes)    |
| Send Firmware Packet   | Send one packet (data + seq + CRC)  | Seq # + payload |
| Retransmit             | Request missing packet              | Seq #           |
| Verify Firmware        | Final signature + CRC check         | None            |
| Jump to App            | Jump to application start           | None            |
| Help                   | List commands                       | None            |

All commands defined in `bootloader/Core/Inc/bootloader_cmds.h`.

---

## Python Host Utilities

Located in `bootloader/util/`

- **`fw-signer.py`** – Signs application binary with AES-CBC-MAC
- **`serial_monitor.py`** – Interactive CLI for bootloader communication
- **`pad_bootloader.py`** – Pad bootloader binary to fixed size
- **`bl_monitor/`** – Modular command implementations

Example usage:
```bash
python util/fw-signer.py --input app.bin --output app_signed.bin
python util/serial_monitor.py 
```

# Bootloader Finite State Machine (FSM)

The bootloader uses a **Finite State Machine (FSM)** to manage the firmware update process reliably. This ensures the update can handle interruptions, errors, and partial transfers without bricking the device.

The FSM is implemented across shared and bootloader-specific files:

- **Shared base**: `common/Src/sm_common.c` and `common/Inc/sm_common.h`  
  Provides a generic state machine framework (states, events, Hierarchical State Machine etc).
  

- **Bootloader-specific**: `bootloader/Core/Src/bootloader_fsm.c` and `bootloader/Core/Inc/bootloader_fsm.h`  
  Defines update-specific states, events, and actions.

This modular design allows reuse if needed elsewhere.

## Generic State Machine Framework (`sm_common.*`)
  **Comms state** - Defines basic state machine that gets the packet. Each packet goes through following state before construction 
  ```c
  status_t comm_state_init(bootloader_fsm_t *me, StateHandler initial);
  status_t comm_state_frame(bootloader_fsm_t *const me, Event const *const e);
  status_t comm_state_id(bootloader_fsm_t *const me, Event const *const e);
  status_t comm_state_length(bootloader_fsm_t *const me, Event const *const e);
  status_t comm_state_payload(bootloader_fsm_t *const me, Event const *const e);
  status_t comm_state_crc(bootloader_fsm_t *const me, Event const *const e);
  ```
  **bootloader\Core\Inc\bootloader_fsm.h** -  defines generic FSM. Each FSM must be inherrited from
  ```c
  typedef status_t (*StateHandler)(Fsm *const me, Event const *const e);
  status_t hsm_top_status(Fsm *const me, Event const *const e);

  struct Fsm {
    StateHandler state;
    StateHandler super_state;
  };

  void Fsm_ctor(Fsm *const me, StateHandler initial);
  void Fsm_init(Fsm *const me, Event const *const e);
  void Fsm_dispatch(Fsm *const me, Event const *const e);
  ```

   

`sm_common` implements a simple table-driven FSM:

- **States** — Defined as enum values.
- **Events** — Triggers (e.g., command received, timeout, error).
- **Actions** — Functions executed on transition.
- **Transition Table** — Array mapping (current_state + event) → (next_state, action).

Key functions:
- `sm_init()` → Initializes FSM to starting state.
- `sm_dispatch_event(event)` → Looks up transition, runs action if any, updates state.
- `sm_get_current_state()` → Returns current state.

This lightweight approach avoids complex switch statements and makes transitions easy to audit.

## Bootloader FSM Definition (`bootloader_fsm.*`)

### States (in `bootloader_fsm.h`)

Typical states in the update sequence:

```c
typedef enum {
    BL_STATE_IDLE,          // Waiting for sync / initial check
    BL_STATE_SYNCED,        // Handshake complete
    BL_STATE_ERASING,       // Erasing app flash
    BL_STATE_READY_FOR_DATA,// Size received, ready for packets
    BL_STATE_RECEIVING,     // Receiving packets
    BL_STATE_VERIFYING,     // Final signature/CRC check
    BL_STATE_COMPLETE,      // Update successful
    BL_STATE_ERROR,         // Fatal error – stay here until reset
    // Additional: BL_STATE_BOOT_APP (transient for jumping)
} bl_fsm_state_t;
```


## Data Flow(`comms fsm + Bootloader fsm`)
- Data is injected to bootloader via serial interface defined in ```bl_serrif.h```
```c
typedef void (*bl_send_bytes_fn_t)(const uint8_t *data, const uint32_t size);
typedef void (*bl_deinit_fn_t)(void);

typedef struct bl_serrif_t {
	bl_send_bytes_fn_t wb;
} bl_serrif_t;

```
- Cortex M4 reads data from DMA and injects into bootloader.
- Bootloader first runs comms state machine
  - comms state machine reads incoming byte stream on byte read event and after detecting correct frame, starts constructing packet.
- If packet is valid (CRC Verified), it is fed into main bootloader_fsm. For invalid packet, bootloader requests retransmit and discards the packet. 
- A bootloader commands must implement it handler
```c
typedef struct bootloader_cmd {
	uint8_t command_id;
	process_packet process;
	bool send_response;
} bootloader_cmd_t;
```
- bootloader detects command ID and checks if valid handle is implemented/supported. 
- If command is not valid, a NACK is sent with error code. If handler exists, the packet is handled independantly.
- Bootloader sends packet to host via same interface.