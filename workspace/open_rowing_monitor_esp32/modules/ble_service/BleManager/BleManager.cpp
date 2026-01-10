#include "BleManager.h"
#include "FTMS.h" // To get UUID definitions

struct bt_conn *BleManager::current_conn = nullptr;

LOG_MODULE_REGISTER(BleManager, LOG_LEVEL_INF);
K_MUTEX_DEFINE(BleManager::conn_mutex);
// GAP Connection Callbacks
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = BleManager::onConnected,
    .disconnected = BleManager::onDisconnected,
};

// -----------------------------------------------------------------------------
// ADVERTISING DATA
// -----------------------------------------------------------------------------
// 1. Flags: General Discoverable
// 2. UUIDs: We MUST advertise the FTMS Service UUID (0x1826)
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_FTMS_VAL)) // 0x1826
};

// Scan Response: Show the Device Name
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void BleManager::init() {
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);// change to LOG_ERR
        return;
    }
    LOG_INF("Bluetooth initialized\n");
    startAdvertising();
}

void BleManager::startAdvertising() {
    // int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return;
    }
    LOG_INF("Advertising successfully started\n");
}

bool BleManager::isConnected() {
    return (current_conn != nullptr);
}

void BleManager::onConnected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        LOG_ERR("Connection failed (err 0x%02x)\n", err);
    } else {
        k_mutex_lock(&conn_mutex, K_FOREVER); // PROTECT THIS
        if (current_conn) {
            bt_conn_unref(current_conn); // Clean up if something was there
        }
        current_conn = bt_conn_ref(conn);
        k_mutex_unlock(&conn_mutex);
        LOG_INF("Connected");
    }
}

void BleManager::onDisconnected(struct bt_conn *conn, uint8_t reason) {
    LOG_INF("Disconnected (reason 0x%02x)", reason);

    k_mutex_lock(&conn_mutex, K_FOREVER);
    // ONLY unref if this is the connection we are actually tracking
    if (current_conn == conn) {
        bt_conn_unref(current_conn);
        current_conn = nullptr;
    }
    k_mutex_unlock(&conn_mutex);
    startAdvertising();
}
struct bt_conn* BleManager::get_connection_ref() {
    struct bt_conn *ref = nullptr;

    // Wait for the lock
    k_mutex_lock(&conn_mutex, K_FOREVER);

    if (current_conn) {
        ref = bt_conn_ref(current_conn);
    }

    k_mutex_unlock(&conn_mutex);
    return ref;
}
