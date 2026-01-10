#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

class BleManager {
public:
    void init();
    static void startAdvertising();

    // Check if a device is currently connected
    bool isConnected();

    static void onConnected(struct bt_conn *conn, uint8_t err);
    static void onDisconnected(struct bt_conn *conn, uint8_t reason);
    struct bt_conn* get_connection_ref();
private:
    // Track the active connection
    static struct bt_conn *current_conn;
    static struct k_mutex conn_mutex;
};

#endif // BLE_MANAGER_H
