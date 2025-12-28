# Fota

Firmware Over-The-Air (FOTA) update implementation for STM32L4 series microcontrollers (ARM Cortex-M4).

## Overview

This repository implements a **secure Firmware Over-The-Air (FOTA)** system for embedded devices. It consists of:

- A **protected bootloader** that handles firmware updates, validation, decryption/verification, and secure boot.
- A **separate updatable application**.
- **Cryptographic security** using AES for signing and verifying firmware images.
- **Python utilities** for signing firmware, padding binaries, and interacting with the bootloader over serial.

The design follows a classic dual-slot architecture:
- Bootloader resides in a fixed, protected flash region.
- Application occupies a separate slot that can be updated remotely.
- On successful update and verification, the bootloader switches to the new application.


**Status** (as of December 28, 2025):
- Early-stage project (9 commits).
- Secure signed firmware support recently added.

**ContributingContributions** are welcome! To contribute:Fork the repository.
- Create a feature branch.
- Submit a pull request with clear descriptions.
- Follow code style (.clang-format provided).

For issues or suggestions, open a GitHub issue.

**License**
This project is licensed under the MIT License. See the LICENSE file for details.Permission is hereby granted, free of charge, to any person obtaining a copy of this software...

## Technical Documentation
For detailed STM32 Cortex-M4 bootloader and application memory layout, see
[CORTEXM4 Documentation](cortex-m4/CORTEXM4.md)