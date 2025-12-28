#include "StorageManager.h"
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(storage_manager, LOG_LEVEL_INF);

#define MOUNT_POINT "/lfs"

const char* StorageManager::mount_pt = MOUNT_POINT;
const char* StorageManager::log_file_path = "/lfs/session.fit";

StorageManager::StorageManager() {
    mp.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition);

    mp.mnt_point = mount_pt;
    mp.fs_data = &lfs_data;
    mp.type = FS_LITTLEFS;
    mp.flags = FS_MOUNT_FLAG_NO_FORMAT;

    // LittleFS Config
    lfs_data.cfg.read_size = 16;
    lfs_data.cfg.prog_size = 16;
    lfs_data.cfg.cache_size = 64;
    lfs_data.cfg.lookahead_size = 32;
    lfs_data.cfg.block_cycles = 512;
}

int StorageManager::init() {
    int res = fs_mount(&mp);

    if (res != 0) {
        LOG_WRN("Mount failed, attempting to format... (Error: %d)", res);
        res = fs_mkfs(FS_LITTLEFS, (uintptr_t)mp.storage_dev, &lfs_data, 0);
        if (res == 0) {
            LOG_INF("Format successful, remounting...");
            res = fs_mount(&mp);
        }
    }

    if (res == 0) {
        LOG_INF("LittleFS mounted at %s", mount_pt);
        isMounted = true;
    } else {
        LOG_ERR("Failed to init LittleFS (Error: %d)", res);
    }
    return res;
}

bool StorageManager::appendRecord(const std::string& data) {
    if (!isMounted) return false;

    struct fs_file_t file;
    fs_file_t_init(&file);

    int rc = fs_open(&file, log_file_path, FS_O_CREATE | FS_O_APPEND | FS_O_WRITE);
    if (rc < 0) {
        LOG_ERR("Failed to open file: %d", rc);
        return false;
    }

    rc = fs_write(&file, data.c_str(), data.size());
    fs_close(&file);

    if (rc < 0) {
        LOG_ERR("Failed to write: %d", rc);
        return false;
    }
    return true;
}

void StorageManager::listMountedVol() {
    if (!isMounted) return;

    struct fs_dir_t dir;
    fs_dir_t_init(&dir);

    int rc = fs_opendir(&dir, mount_pt);
    if (rc < 0) {
        LOG_ERR("Error opening dir: %d", rc);
        return;
    }

    LOG_INF("Listing files in %s:", mount_pt);
    struct fs_dirent entry;
    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != 0) {
        LOG_INF("  %c %s (%u bytes)",
            entry.type == FS_DIR_ENTRY_DIR ? 'D' : 'F',
            entry.name,
            entry.size);
    }
    fs_closedir(&dir);
}
