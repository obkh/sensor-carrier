#include "sht31.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t response[6];
    int fail_write;
    int fail_read;
    unsigned writes;
} mock_bus_t;

static int mock_write(void *context, uint8_t address, const uint8_t *data,
                      size_t length)
{
    mock_bus_t *mock = context;
    assert(address == SHT31_ADDRESS && length == 2);
    assert(data[0] == 0x24 && data[1] == 0x00);
    ++mock->writes;
    return mock->fail_write;
}

static int mock_read(void *context, uint8_t address, uint8_t *data,
                     size_t length)
{
    mock_bus_t *mock = context;
    assert(address == SHT31_ADDRESS && length == 6);
    if (mock->fail_read) return mock->fail_read;
    memcpy(data, mock->response, 6);
    return 0;
}

int main(void)
{
    const uint8_t known[] = {0xbe, 0xef};
    assert(sht31_crc8(known, 2) == 0x92);
    mock_bus_t mock = { .response = {0x66, 0x66, 0, 0x80, 0x00, 0} };
    mock.response[2] = sht31_crc8(mock.response, 2);
    mock.response[5] = sht31_crc8(mock.response + 3, 2);
    sht31_bus_t bus = { .context = &mock, .write = mock_write,
                        .read = mock_read };
    sht31_sample_t sample = {0};
    assert(sht31_start_measurement(&bus) == SHT31_OK && mock.writes == 1);
    assert(sht31_read_measurement(&bus, &sample) == SHT31_OK);
    assert(sample.temperature_millideg_c == 25000);
    assert(sample.humidity_millipercent >= 50000 &&
           sample.humidity_millipercent <= 50001);
    mock.response[2] ^= 1;
    assert(sht31_read_measurement(&bus, &sample) == SHT31_CRC_ERROR);
    mock.fail_read = -1;
    assert(sht31_read_measurement(&bus, &sample) == SHT31_BUS_ERROR);
    mock.fail_write = -1;
    assert(sht31_start_measurement(&bus) == SHT31_BUS_ERROR);
    assert(sht31_start_measurement(NULL) == SHT31_INVALID_ARGUMENT);
    puts("SHT31 driver tests passed");
    return 0;
}
