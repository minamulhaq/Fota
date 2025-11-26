import sys
import time
from typing import Optional

from serial import Serial
from serial.tools import list_ports

BAUDRATE = 115200
KEYWORD = "STM"


class CmdLineSender:
    def __init__(self, num: int) -> None:
        self.port: Optional[Serial] = None
        self.num = num
        self.scan_com_ports()
        self.run()

    def run(self):
        try:
            assert self.port
            assert isinstance(self.num, int)

            PACKET = bytes([self.num])
            print(f"Sending packing : {PACKET}")
            self.port.write(PACKET)
            time.sleep(0.5)
            while self.port.in_waiting:
                out = self.port.readline().decode('utf-8')
                print(out)
        except Exception as e:
            raise e

    def scan_com_ports(self) -> None | Serial:
        for port in list_ports.comports():
            if KEYWORD in port.description:
                print(port)
                self.port = Serial(port.device, BAUDRATE, timeout=1)
                return self.port

    def __exit__(self):
        if self.port and self.port.is_open:
            self.port.close()


if __name__ == "__main__":
    """
    0xb1 0x0 0x6 0x8d 0x7 0xbc

    """

    if len(sys.argv) < 2:
        print("no byte passed\n")
        sys.exit(-1)
    num = int(sys.argv[1], 16)
    cmd = CmdLineSender(num=num)
