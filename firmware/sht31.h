#ifndef SHT31_H
#define SHT31_H

#include <stddef.h>
#include <stdint.h>

#define SHT31_ADDRESS 0x44u

typedef int (*sht31_write_fn)(void *context, uint8_t address,
                              const uint8_t *data, size_t length);
typedef int (*sht31_read_fn)(void *context, uint8_t address,
                             uint8_t *data, size_t length);

typedef struct {
    void *context;
    sht31_write_fn write;
    sht31_read_fn read;
} sht31_bus_t;

typedef struct {
    int32_t temperature_millideg_c;
    uint32_t humidity_millipercent;
} sht31_sample_t;

typedef enum {
    SHT31_OK = 0,
    SHT31_INVALID_ARGUMENT,
    SHT31_BUS_ERROR,
    SHT31_CRC_ERROR
} sht31_result_t;

/* Issue a single-shot, high-repeatability measurement command. */
sht31_result_t sht31_start_measurement(const sht31_bus_t *bus);
/* Call after at least 15 ms; this function does not delay. */
sht31_result_t sht31_read_measurement(const sht31_bus_t *bus,
                                      sht31_sample_t *sample);
uint8_t sht31_crc8(const uint8_t *data, size_t length);

#endif
