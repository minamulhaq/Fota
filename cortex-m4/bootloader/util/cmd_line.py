import time
from typing import Optional

from serial import Serial
from serial.tools import list_ports

BAUDRATE = 115200
KEYWORD = "STM"


class CmdLineSender:
    def __init__(self) -> None:
        self.port: Optional[Serial] = None
        self.scan_com_ports()

    def run(self, nums: list[int]):
        try:
            for i, n in enumerate(nums):
                print(f"Sending byte: {i + 1} / {len(nums)}")

                assert self.port
                assert isinstance(n, int)

                PACKET = bytes([n])
                print(f"Sending packing : {[hex(x) for x in PACKET]}")
                self.port.write(PACKET)
                # time.sleep(0.01)
                input("Enter to send next byte")
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
    nums1 = [0xB1, 0x0, 0x6, 0x8D, 0x7, 0xBC]
    nums2 = [0xB0, 0x00, 0x47, 0xBC, 0x1C, 0xA5]
    cont = True
    cmd = CmdLineSender()
    while cont:
        cmd.run(nums1)
        print("cmd1 sent")
        input("Enter to Send command 2? ")
        cmd.run(nums2)
        cont = False
        x = input("Continue? [y/n]").lower()
        cont = True if x == "y" else False
