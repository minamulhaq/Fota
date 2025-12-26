class CRCCalculator:
    @staticmethod
    def crc32_stm32_style(data: bytes) -> int:
        """
        STM32 CRC peripheral compatible
        CRC-32 / MPEG-2
        Poly: 0x04C11DB7
        Init: 0xFFFFFFFF
        No reflection
        No final XOR
        """
        crc = 0xFFFFFFFF
        poly = 0x04C11DB7

        for byte in data:
            crc ^= byte << 24  # align byte to MSB
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
                else:
                    crc = (crc << 1) & 0xFFFFFFFF

        return crc
