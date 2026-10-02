idf_component_register(
    SRCS "main.c" "../../firmware/sht31.c"
    INCLUDE_DIRS "../../firmware"
    REQUIRES esp_driver_i2c
)
