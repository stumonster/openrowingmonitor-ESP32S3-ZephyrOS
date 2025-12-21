#include "StorageManager.h"
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(StorageManager, LOG_LEVEL_INF);

// We need the numeric ID of the partition labeled "storage" in Device Tree
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(storage_partition)

const char* StorageManager::mount_pt = "/lfs";
const char* StorageManager::log_file_path = "/lfs/workout.csv";

StorageManager::StorageManager() {
    // 1. Configure LittleFS parameters
    // Zephyr calculates the read/write/erase sizes automatically
    // from the flash driver if we set these to 0.
    lfs_data.read_size = 16;
    lfs_data.prog_size = 16;
    lfs_data.cache_size = 64;
    lfs_data.lookahead_size = 32;
    lfs_data.block_cycles = 512;

    // 2. Setup the Mount Point
    mp.type = FS_LITTLEFS;
    mp.fs_data = &lfs_data;
    mp.mnt_point = mount_pt;

    // 3. Point to the specific Flash Partition
    mp.storage_dev = (void*)STORAGE_PARTITION_ID;
}

int StorageManager::init() {
    LOG_INF("Mounting LittleFS to %s...", mp.mnt_point);

    int res = fs_mount(&mp);

    // 4. Handle First-Boot Scenario (Format required)
    if (res < 0 && res != -EBUSY) {
        LOG_WRN("Mount failed (Error %d). Attempting to format...", res);

        // Use the generic FS API to format the area defined in 'mp'
        res = fs_format(&mp);
        if (res < 0) {
            LOG_ERR("Format failed: %d", res);
            return res;
        }

        // Try mounting again after format
        res = fs_mount(&mp);
    }

    if (res == 0 || res == -EBUSY) {
        LOG_INF("Storage Ready.");
        isMounted = true;
        return 0;
    }

    LOG_ERR("Critical Storage Failure: %d", res);
    return res;
}

// appendRecord remains largely the same, just ensure you check isMounted.
bool StorageManager::appendRecord(const std::string& data) {
    if (!isMounted) return false;

    struct fs_file_t file;
    fs_file_t_init(&file);

    int rc = fs_open(&file, log_file_path, FS_O_CREATE | FS_O_APPEND | FS_O_WRITE);
    if (rc < 0) return false;

    fs_write(&file, data.c_str(), data.length());
    fs_write(&file, "\n", 1);
    fs_close(&file);

    return true;
}

void StorageManager::listMountedVolume() {
    int index = 0;
    const char *mnt_name;
    int rc;

    LOG_INF("--- Inspecting Mounted Volumes ---");
    while (true) {
        rc = fs_readmount(&index, &mnt_name);
        if (rc < 0) {
            // -ENOENT means we reached the end of the list
            break;
        }
        LOG_INF("  [%d] Mounted: %s", index, mnt_name);
    }
    LOG_INF("----------------------------------");
}
