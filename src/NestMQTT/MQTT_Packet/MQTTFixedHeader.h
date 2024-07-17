// #ifndef MQTT_FIXED_HEADER_H_
// #define MQTT_FIXED_HEADER_H_

// #include "MQTTCore.h"
// #include <cstdint>
// using namespace MQTTCore;

// namespace MQTTPacket {

// class FixedHeader {
// private:
//   PacketType control_type;
//   uint8_t control_flags;
//   uint32_t remaining_length;

// public:
//   FixedHeader(MQTTCore::PacketType type, MQTTCore::HeaderFlag flags,
//               uint32_t length, uint32_t payload_size = 0)
//       : control_type(type), control_flags(flags.PUBLISH_QOS0),
//         remaining_length(length + payload_size) {}

//   uint8_t *serialize() {
//     // Calculate the number of bytes needed for the remaining length field
//     uint8_t remainingLengthSize = remainingLengthFieldSize(remaining_length);

//     // Allocate memory for the fixed header
//     uint8_t *header = new uint8_t[1 + remainingLengthSize];

//     // Fill control byte
//     header[0] = (static_cast<uint8_t>(control_type) << 4) | control_flags;

//     // Encode remaining length
//     uint8_t bytesNeeded = encodeRemainingLength(remaining_length, &header[1]);

//     return header;
//   }

//   ~FixedHeader() {}

// private:
//   uint8_t encodeRemainingLength(uint32_t remainingLength,
//                                 uint8_t *destination) {
//     uint8_t currentByte = 0;
//     uint8_t bytesNeeded = 0;

//     do {
//       uint8_t encodedByte = remainingLength % 128;
//       remainingLength /= 128;
//       // if there are more data to encode than 8 bit or 128, set the top bit of
//       // this byte
//       if (remainingLength > 0) {
//         // encodedByte is the LSB and 128 is to set the continuation bit
//         encodedByte = encodedByte | 128;
//       }
//       destination[currentByte++] = encodedByte;
//       bytesNeeded++;
//     } while (remainingLength > 0);

//     return bytesNeeded;
//   }

//   uint8_t remainingLengthFieldSize(uint32_t remainingLength) {
//     if (remainingLength < 128)
//       return 1;
//     if (remainingLength < 16384)
//       return 2;
//     if (remainingLength < 2097152)
//       return 3;
//     if (remainingLength > 268435455)
//       return 0;
//     return 4;
//   }
// };

// } // namespace MQTTPacket

// #endif