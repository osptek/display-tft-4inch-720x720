set(srcs "main.c")
set(include_dirs ".")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES hw_init
)

spiffs_create_partition_assets(
    eaf
    "../assets"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".eaf"
)