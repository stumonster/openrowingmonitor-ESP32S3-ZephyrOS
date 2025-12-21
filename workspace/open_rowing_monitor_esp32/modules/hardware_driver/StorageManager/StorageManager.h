#pragma once

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h> // <--- NEW HEADER
// #include <lfs.h>
#include <string>

class StorageManager {
public:
    StorageManager();
    int init();
    bool appendRecord(const std::string& data);
    void listMountedVol();

private:
    // LittleFS specific structures
    struct fs_mount_t mp;
    struct fs_littlefs lfs_data; // <--- Configuration for LittleFS

    static const char* mount_pt;
    static const char* log_file_path;

    bool isMounted = false;
};
