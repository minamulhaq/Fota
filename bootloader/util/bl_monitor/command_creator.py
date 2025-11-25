import time
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum

from serial import Serial


class CommandIDs(Enum):
    B_ACK = 0xE0
    B_NACK = 0xE1
    B_CMD_GET_VERSION = 0xB1
    B_CMD_GET_HELP = 0xB2
    B_CMD_GET_CID = 0xB3
    B_CMD_GET_RDP_LVL = 0xB4
    B_CMD_JMP_TO_ADDR = 0xB5
    B_CMD_ERASE_FLASH = 0xB6


@dataclass
class Packet:
    id: int
    length: int = 0
    payload: None | list[int] = None
    crc32: int = 0

    def __post_init__(self):
        if self.payload is None:
            self.payload = []


@dataclass
class CommandInfo:
    id: int
    nemonic: str
    description: str

    def __str__(self) -> str:
        response = f"Command: {self.nemonic}\n"
        response += f"ID: {self.id}\n"
        response += f"Description: {self.description}"

        return response


class Command(ABC):
    @property
    @abstractmethod
    def packet(self) -> Packet: ...


    def validate_id(self, id: int)-> bool:
        return self.packet.id == id

    @property
    @abstractmethod
    def info(self) -> CommandInfo: ...

    def compute_crc32_for_packet(self, pkt: Packet) -> int:
        # Build data exactly like STM32 expects
        data = bytearray()
        data.append(pkt.id & 0xFF)
        data.append(pkt.length & 0xFF)

        if pkt.length > 0 and pkt.payload:
            data.extend([b & 0xFF for b in pkt.payload[: pkt.length]])

        return self.crc32_stm32_style(data)

    def crc32_stm32_style(self, data: bytes) -> int:
        crc = 0xFFFFFFFF

        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc >>= 1

        return (~crc) & 0xFFFFFFFF

    @property
    def len_crc(self) -> int:
        return 4

    @property
    def cmd(self) -> bytearray:
        """Generate raw byte array to send: id + length + payload + crc (little-endian)"""
        pkt: Packet = self.packet
        pkt.crc32 = self.compute_crc32_for_packet(pkt)
        raw = bytearray([pkt.id & 0xFF, pkt.length & 0xFF])
        if pkt.length > 0:
            assert pkt.payload, "Payload can't be None"
            raw.extend([b & 0xFF for b in pkt.payload])
        raw.extend(pkt.crc32.to_bytes(4, byteorder="little"))
        return raw


# ============================================================================
# COMMAND IMPLEMENTATIONS
# ============================================================================

class CommandGetBootloaderVersion(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_VERSION.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_VERSION.value,
            description=f"ID: {CommandIDs.B_CMD_GET_VERSION.value} | {CommandIDs.B_CMD_GET_VERSION.value:#04X} | Get Bootloader Version",
            nemonic="CommandGetBootloaderVersion",
        )


class CommandGetHelp(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_HELP.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_HELP.value,
            description=f"ID: {CommandIDs.B_CMD_GET_HELP.value} | {CommandIDs.B_CMD_GET_HELP.value:#04X} | Get Supported Commands",
            nemonic="CommandGetHelp",
        )


class CommandGetChipID(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_CID.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_CID.value,
            description=f"ID: {CommandIDs.B_CMD_GET_CID.value} | {CommandIDs.B_CMD_GET_CID.value:#04X} | Get Chip ID",
            nemonic="CommandGetChipID",
        )


class CommandGetRDPLevel(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_RDP_LVL.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_RDP_LVL.value,
            description=f"ID: {CommandIDs.B_CMD_GET_RDP_LVL.value} | {CommandIDs.B_CMD_GET_RDP_LVL.value:#04X} | Get Read Protection Level",
            nemonic="CommandGetRDPLevel",
        )


class CommandJumpToAddress(Command):
    def __init__(self, address: int):
        self.address = address

    @property
    def packet(self) -> Packet:
        # Address as 4 bytes little-endian
        payload = list(self.address.to_bytes(4, byteorder="little"))
        return Packet(id=CommandIDs.B_CMD_JMP_TO_ADDR.value, length=4, payload=payload)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_JMP_TO_ADDR.value,
            description=f"ID: {CommandIDs.B_CMD_JMP_TO_ADDR.value} | {CommandIDs.B_CMD_JMP_TO_ADDR.value:#04X} | Jump to Address: 0x{self.address:08X}",
            nemonic="CommandJumpToAddress",
        )


class CommandEraseFlash(Command):
    def __init__(self, sectors: list[int] | None = None, mass_erase: bool = False):
        self.sectors = sectors if sectors else []
        self.mass_erase = mass_erase

    @property
    def packet(self) -> Packet:
        if self.mass_erase:
            payload = [0xFF, 0xFF]
            length = 2
        else:
            payload = [len(self.sectors)] + self.sectors
            length = len(payload)

        return Packet(id=CommandIDs.B_CMD_ERASE_FLASH.value, length=length, payload=payload)

    @property
    def info(self) -> CommandInfo:
        erase_type = "Mass Erase" if self.mass_erase else f"Erase Sectors: {self.sectors}"
        return CommandInfo(
            id=CommandIDs.B_CMD_ERASE_FLASH.value,
            description=f"ID: {CommandIDs.B_CMD_ERASE_FLASH.value} | {CommandIDs.B_CMD_ERASE_FLASH.value:#04X} | {erase_type}",
            nemonic="CommandEraseFlash",
        )
