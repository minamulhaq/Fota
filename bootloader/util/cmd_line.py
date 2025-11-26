import time
from typing import Optional

from serial import Serial
from serial.tools import list_ports

BAUDRATE = 115200
KEYWORD = "STM"


class CmdLineSender:
    def __init__(self, nums: list[int]) -> None:
        self.port: Optional[Serial] = None
        self.nums = nums
        self.scan_com_ports()
        self.run()

    def run(self):
        try:
            for i, n in enumerate(self.nums):
                print(f"Sending byte: {i + 1} / {len(self.nums) }")

                assert self.port
                assert isinstance(n, int)

                PACKET = bytes([n])
                print(f"Sending packing : {PACKET}")
                self.port.write(PACKET)
                time.sleep(0.5)
                while self.port.in_waiting:
                    out = self.port.readline().decode("utf-8")
                    print(out)
                x = input()
        except Exception as e:
            raise e

    def scan_com_ports(self) -> None | Serial:
        for port in list_ports.comports():
            if KEYWORD in port.description:
                print(port)
                self.port = Serial(port.device, BAUDRATE)
                return self.port

    def __exit__(self):
        if self.port and self.port.is_open:
            self.port.close()


if __name__ == "__main__":
    nums = [0xB1, 0x0, 0x6, 0x8D, 0x7, 0xBC]
    cont = True
    while cont:
        cont = False
        cmd = CmdLineSender(nums=nums)
        x = input("Continue? [y/n]").lower()
        cont = True if x == 'y' else False
