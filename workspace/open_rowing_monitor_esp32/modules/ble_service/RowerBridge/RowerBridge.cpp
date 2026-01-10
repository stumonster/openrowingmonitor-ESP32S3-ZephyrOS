#include "RowerBridge.h"
#include <zephyr/kernel.h> // For k_uptime_get()

RowerBridge::RowerBridge(RowingEngine& engine, FTMS& service, BleManager& blemanager)
    : m_engine(engine), m_service(service), m_blemanager(blemanager) {}

void RowerBridge::update() {
    // 1. Check if enough time has passed (Rate Limiting)
    uint32_t now = k_uptime_get_32();
    if ((now - last_update_time) < UPDATE_INTERVAL_MS) {
        return;
    }
    last_update_time = now;

    struct bt_conn *conn = m_blemanager.get_connection_ref();
    if(conn) {
        // 2. Get Fresh Data from Physics Engine
        RowingData data = m_engine.getData();

        // 3. Send to BLE Service
        // The Service handles the check for "isNotifyEnabled", so we can just call it.
        m_service.notifyRowingData(conn, data);
        bt_conn_unref(conn);
    }
}
