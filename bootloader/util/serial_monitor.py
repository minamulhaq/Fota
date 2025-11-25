from enum import Enum
import os
import time
from typing import Optional

from bl_monitor import Command, CommandGetBootloaderVersion
from serial import Serial
from serial.tools import list_ports

BAUDRATE = 115200
KEYWORD = "STM32"


class PrintMode(Enum):
    STRING_MODE = 0x00
    BYTES_MODE = 0x01


startup_message = """
+---------------------------------------+
|    STM32 Bootloader Communication     |
+---------------------------------------+

Enter Decimal ID for any of following Supported Commands:

"""

SUPPORTED_COMMANDS: list[Command] = [CommandGetBootloaderVersion()]


class SerialMointor:
    def __init__(self, print_mode: PrintMode = PrintMode.STRING_MODE) -> None:
        self.print_mode: PrintMode = print_mode
        self.SupportedCommands: list[Command] = SUPPORTED_COMMANDS
        self.port: Optional[Serial] = None
        self.run()

    def scan_com_ports(self) -> None | Serial:
        for port in list_ports.comports():
            if KEYWORD in port.description:
                print(port)
                port = Serial(port.device, BAUDRATE, timeout=1)
                return port
        return None

    @property
    def startup_message(self):
        cmd_strs = ""
        for cmd in self.SupportedCommands:
            cmd_strs += cmd.info.description
        return startup_message + cmd_strs + "\n\nEnter command ID to execute: "

    def connect(self):
        try:
            if self.port is None:
                self.port = self.scan_com_ports()
                print("No port found")

            if self.port:
                self.port.open()
                return True
            return False
        except Exception as e:
            raise e

    def read_response_string_mode(self) -> str | None:
        assert self.port, "Serial port not initialized"

        if self.port.in_waiting > 0:
            response_bytes = self.port.read(self.port.in_waiting)
            try:
                response_str = response_bytes.decode("utf-8", errors="replace")
            except Exception as e:
                print(f"Failed to decode response: {e}")
                response_str = None
            if response_str:
                print(f"Response (string mode): {response_str.strip()}")
            return response_str
        return None


    def read_response_bytes_mode(self):
        assert self.port
        if self.port.in_waiting > 0:
            response = self.port.read(self.port.in_waiting)  # Use read, not readline
            print(f"Response: {' '.join([hex(x) for x in response])}")
            return bytes(response)

    def send_bootloader_cmd(self, cmd: Command) -> bytes | None:
        print(
            f"Executing Command:\n{cmd.info}\ncmd raw bytes : {' '.join([hex(x) for x in cmd.cmd])}\n"
        )

        assert self.port

        self.port.reset_input_buffer()
        self.port.reset_output_buffer()

        self.port.write(cmd.cmd)
        time.sleep(0.1)
        if self.print_mode is PrintMode.STRING_MODE:
            self.read_response_string_mode()
        elif self.print_mode is PrintMode.BYTES_MODE:
            self.read_response_bytes_mode()


    def run(self) -> None:
        while True:
            try:
                # os.system("cls" if os.name == "nt" else "clear")
                if not self.port:
                    self.port = self.scan_com_ports()

                    if not self.port:
                        print("Device not connected")
                        break

                if self.port and not self.port.is_open:
                    self.connect()

                cmd_id = input(self.startup_message)

                if not cmd_id:
                    continue
                os.system("cls" if os.name == "nt" else "clear")
                valid_cmd: Optional[Command] = None
                for cmd in self.SupportedCommands:
                    if cmd.validate_id(int(cmd_id)):
                        valid_cmd = cmd

                if not valid_cmd:
                    print("Invalid Command")
                else:
                    self.send_bootloader_cmd(valid_cmd)

                input("\n\npress Enter key to continue")
            except Exception as e:
                print(e)
                input()


if __name__ == "__main__":
    sm = SerialMointor()
