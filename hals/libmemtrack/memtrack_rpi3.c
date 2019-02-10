#include <errno.h>

#include <hardware/hardware.h>
#include <hardware/memtrack.h>

int rpi3_memtrack_init(const struct memtrack_module *module)
{
    if (!module)
        return -1;

    return 0;
}

static struct hw_module_methods_t memtrack_module_methods = {
    .open = NULL,
};

struct memtrack_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = MEMTRACK_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = MEMTRACK_HARDWARE_MODULE_ID,
        .name = "RPi3 Memory Tracker HAL",
        .author = "brobwind.com",
        .methods = &memtrack_module_methods,
    },

    .init = rpi3_memtrack_init,
};
