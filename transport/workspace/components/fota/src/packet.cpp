
#include <iostream>
#include <iomanip>
#include "packet.hpp"
#include "esp_rom_crc.h"

#include "esp_rom_crc.h"
#include <cstring>

uint32_t Packet::calculate_packet_crc() const
{

   // Build buffer: ID + LENGTH + payload
   uint8_t buffer[2 + MAX_PAYLOAD_SIZE];
   size_t idx = 0;
   buffer[idx++] = id;
   buffer[idx++] = length;
   if (length > 0)
   {
      std::memcpy(&buffer[idx], payload, length);
      idx += length;
   }

   // STM32 CRC-32 / MPEG-2 style
   uint32_t crc = 0xFFFFFFFF;
   const uint32_t poly = 0x04C11DB7;

   for (size_t i = 0; i < idx; ++i)
   {
      crc ^= static_cast<uint32_t>(buffer[i]) << 24; // MSB first
      for (int bit = 0; bit < 8; ++bit)
      {
         if (crc & 0x80000000)
            crc = (crc << 1) ^ poly;
         else
            crc <<= 1;
      }
   }
   return crc; 
}

std::ostream &operator<<(std::ostream &os, const Packet &p)
{
   std::ios_base::fmtflags f(os.flags());
   char prev_fill = os.fill();

   os << "ID: 0x"
      << std::uppercase << std::hex
      << std::setw(2) << std::setfill('0')
      << static_cast<uint32_t>(p.id) << "\n";

   os << std::dec << "LENGTH: "
      << static_cast<uint32_t>(p.length) << "\n";

   os << "PAYLOAD: [";
   for (size_t i = 0; i < p.length; ++i)
   {
      os << "0x"
         << std::setw(2) << std::setfill('0')
         << std::hex
         << static_cast<uint32_t>(p.payload[i]);

      if (i + 1 < p.length)
         os << ", ";
   }
   os << "]\n";

   os << "CRC: 0x"
      << std::setw(8) << std::setfill('0')
      << std::hex << p.crc32 << "\n";

   os.flags(f);
   os.fill(prev_fill);
   return os;
}
