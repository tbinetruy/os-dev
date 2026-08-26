#include <vmm.h>

#include <errno.h>

int vmm_direct_phys_to_virt(uint32_t phys, uint32_t *virt_out)
{
    if (virt_out == NULL || phys >= DIRECT_MAP_PHYS_LIMIT) {
        return -EINVAL;
    }

    *virt_out = DIRECT_MAP_START + phys;
    return 0;
}

int vmm_direct_virt_to_phys(uint32_t virt, uint32_t *phys_out)
{
    if (phys_out == NULL || virt < DIRECT_MAP_START ||
        virt >= DIRECT_MAP_END_EXCLUSIVE) {
        return -EINVAL;
    }

    *phys_out = virt - DIRECT_MAP_START;
    return 0;
}
