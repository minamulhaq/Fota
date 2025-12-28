from abc import ABC, abstractmethod
from pathlib import Path
import struct
import subprocess
import sys

from bl_monitor.crc_calculator import CRCCalculator


APPLICATION_START_OFFSET = 0x800
SIGNING_KEY = "000102030405060708090A0B0C0D0E0F"
ZEROED_IV = "00000000000000000000000000000000"


class InfoField(ABC):
    @classmethod
    @abstractmethod
    def start_idx(cls) -> int: ...

    @classmethod
    @abstractmethod
    def size(cls) -> int: ...

    @classmethod
    def end_idx(cls) -> int:
        return cls.start_idx() + cls.size()


# ───────── fw_info_t ─────────


class FW_VERSION(InfoField):
    @classmethod
    def start_idx(cls) -> int:
        return 0x00

    @classmethod
    def size(cls) -> int:
        return 4


class FW_PADDING(InfoField):  # padding[2]
    @classmethod
    def start_idx(cls) -> int:
        return 0x04

    @classmethod
    def size(cls) -> int:
        return 8


class FW_APP_SIZE(InfoField):
    @classmethod
    def start_idx(cls) -> int:
        return 0x0C

    @classmethod
    def size(cls) -> int:
        return 4


class FW_INFO_T(InfoField):
    @classmethod
    def start_idx(cls) -> int:
        return FW_VERSION.start_idx()

    @classmethod
    def size(cls) -> int:
        return 16


# ───────── fota_shared_t ─────────


class FW_SIGNATURE(InfoField):
    @classmethod
    def start_idx(cls) -> int:
        return 0x10

    @classmethod
    def size(cls) -> int:
        return 16


class FW_CRC(InfoField):
    @classmethod
    def start_idx(cls) -> int:
        return 0x20

    @classmethod
    def size(cls) -> int:
        return 4


class FW_SENTINAL(InfoField):
    @classmethod
    def start_idx(cls) -> int:
        return 0x2C

    @classmethod
    def size(cls) -> int:
        return 4


class FirmwareSigner:
    def __init__(self, binfile: Path) -> None:
        self.bin_file: Path = binfile
        self.raw_bytes: bytearray = self.read_original_file()
        self.process()

    @property
    def fileName_bin_to_sign(self) -> Path:
        return self.bin_file.parent.joinpath("app_to_sign.bin")

    @property
    def fileName_encrypted_bin(self) -> Path:
        return self.bin_file.parent.joinpath("app_encrypted.bin")

    def update_fw_size(self) -> None:
        fw_length = len(self.raw_bytes[APPLICATION_START_OFFSET:])
        print(f"FW length is: {int(fw_length)} (0x{fw_length:X}) bytes")
        self.raw_bytes[FW_APP_SIZE.start_idx() : FW_APP_SIZE.end_idx()] = struct.pack(
            "<I", fw_length
        )
        
    def append_length_plus_app(self):
        app_size_bytes = self.raw_bytes[FW_INFO_T.start_idx() : FW_INFO_T.end_idx()]
        orignal_fw = self.raw_bytes[APPLICATION_START_OFFSET:]
        bytes_for_bin_to_sign = app_size_bytes + orignal_fw

        print(f"writing {self.fileName_bin_to_sign.as_posix()}")
        with open(self.fileName_bin_to_sign, "wb") as f:
            f.write(bytes_for_bin_to_sign)

    def encrypt_binary(self):
        print(f"Creating encrypted binary file: {self.fileName_encrypted_bin}")
        openssl_command = f"openssl enc -aes-128-cbc -nosalt -K {SIGNING_KEY} -iv {ZEROED_IV} -in {self.fileName_bin_to_sign} -out {self.fileName_encrypted_bin}"
        subprocess.call(openssl_command.split(" "))

    def get_signature(self):
        with open(self.fileName_encrypted_bin, "rb") as f:
            return f.read()[-16:]

    def append_signature_in_original_image(self, signature: bytes):

        self.raw_bytes[FW_SIGNATURE.start_idx() : FW_SIGNATURE.end_idx()] = signature
        app_crc = CRCCalculator.crc32_stm32_style(
            self.raw_bytes[APPLICATION_START_OFFSET:]
        )

        print(f"calculated crc: {[f'0x{x:02X}' for x in struct.pack('<I', app_crc)]}")

        self.raw_bytes[FW_CRC.start_idx() : FW_CRC.end_idx()] = struct.pack(
            "<I", app_crc
        )
        with open(self.bin_file, "wb") as f:
            f.write(self.raw_bytes)
        print(
            f"CBC-MAC: {[f'{x:02X}' for x in signature]} and CRC: {app_crc: 0X} | appended to file : {self.bin_file.absolute().as_posix()}"
        )

    def process(self) -> None:
        if self.raw_bytes:
            self.update_fw_size()
            self.append_length_plus_app()
            self.encrypt_binary()
            signature: bytes = self.get_signature()
            self.append_signature_in_original_image(signature=signature)

    def read_original_file(self) -> bytearray:
        print(f"reading file:\t {self.bin_file.absolute().as_posix()}")
        raw_bytes: bytearray
        with open(self.bin_file, "rb") as f:
            raw_bytes = bytearray(f.read())
        print(f"bytes read {len(raw_bytes)} | (0x{len(raw_bytes):X})")
        return raw_bytes

    # def read_original_file(self) -> bytearray:
    #     print(f"reading file {self.bin_file}")

    #     with open(self.bin_file, "rb") as f:
    #         raw_bytes = bytearray(f.read())

    #     total_len = len(raw_bytes)
    #     fw_len = total_len - APPLICATION_START_OFFSET

    #     if fw_len < 0:
    #         raise ValueError("Binary smaller than APPLICATION_START_OFFSET")

    #     pad_len = (-fw_len) % 16  # zero if already aligned

    #     if pad_len:
    #         print(f"Padding firmware with {pad_len} zero bytes for CBC-MAC")
    #         raw_bytes.extend(b"\x00" * pad_len)

    #     print(f"bytes read (after padding): {len(raw_bytes)}")
    #     return raw_bytes


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python fw-signer.py [bin_file]")
        sys.exit(-1)

    bin_file_path = Path(sys.argv[1])
    assert bin_file_path.exists(), f"File {bin_file_path} doesn't exist"

    fs = FirmwareSigner(binfile=bin_file_path)
