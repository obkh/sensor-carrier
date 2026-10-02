#include "sht31.h"

uint8_t sht31_crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0xffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x80u) crc = (uint8_t)(((unsigned)crc << 1) ^ 0x31u);
            else crc = (uint8_t)((unsigned)crc << 1);
        }
    }
    return crc;
}

sht31_result_t sht31_start_measurement(const sht31_bus_t *bus)
{
    static const uint8_t command[] = {0x24u, 0x00u};
    if (!bus || !bus->write) return SHT31_INVALID_ARGUMENT;
    return bus->write(bus->context, SHT31_ADDRESS, command, sizeof command) == 0
               ? SHT31_OK : SHT31_BUS_ERROR;
}

sht31_result_t sht31_read_measurement(const sht31_bus_t *bus,
                                      sht31_sample_t *sample)
{
    uint8_t data[6];
    if (!bus || !bus->read || !sample) return SHT31_INVALID_ARGUMENT;
    if (bus->read(bus->context, SHT31_ADDRESS, data, sizeof data) != 0)
        return SHT31_BUS_ERROR;
    if (sht31_crc8(data, 2) != data[2] || sht31_crc8(data + 3, 2) != data[5])
        return SHT31_CRC_ERROR;
    uint32_t raw_temperature = ((uint32_t)data[0] << 8) | data[1];
    uint32_t raw_humidity = ((uint32_t)data[3] << 8) | data[4];
    sample->temperature_millideg_c = -45000 +
        (int32_t)((175000ull * raw_temperature + 32767ull) / 65535ull);
    sample->humidity_millipercent =
        (uint32_t)((100000ull * raw_humidity + 32767ull) / 65535ull);
    return SHT31_OK;
}
