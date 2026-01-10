/* client/lib/BleClient.js */
export class BleClient {
  constructor(updateStateCallback) {
    this.device = null;
    this.server = null;
    this.updateState = updateStateCallback;
    this.FTMS_SERVICE = 0x1826;
    this.ROWER_DATA_CHAR = 0x2ad1;
  }

  async connect() {
    try {
      console.log("Requesting Bluetooth Device...");
      this.device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [this.FTMS_SERVICE] }],
      });

      this.device.addEventListener(
        "gattserverdisconnected",
        this.onDisconnected.bind(this),
      );

      console.log("Connecting to GATT Server...");
      this.server = await this.device.gatt.connect();

      console.log("Getting Service...");
      const service = await this.server.getPrimaryService(this.FTMS_SERVICE);

      console.log("Getting Characteristic...");
      const characteristic = await service.getCharacteristic(
        this.ROWER_DATA_CHAR,
      );

      console.log("Starting Notifications...");
      await characteristic.startNotifications();
      characteristic.addEventListener(
        "characteristicvaluechanged",
        this.handleRowerData.bind(this),
      );

      console.log("Connected!");
      this.updateState({ connected: true });
    } catch (error) {
      console.error("Connection failed!", error);
    }
  }

  onDisconnected(event) {
    console.log("Device disconnected");
    this.updateState({ connected: false });
    // Note: Web Bluetooth cannot auto-reconnect due to security.
    // User must click "Connect" again.
  }

  handleRowerData(event) {
    const value = event.target.value;

    // Parse logic matching your FTMS.cpp structure
    // [0-1] Flags (Uint16)
    // [2]   Inst Stroke Rate (Uint8)
    // [3-4] Stroke Count (Uint16)
    // [5]   Avg Stroke Rate (Uint8)
    // [6-8] Total Distance (Uint24)
    // [9-10] Inst Pace (Uint16)
    // [11-12] Avg Pace (Uint16)
    // [13-14] Inst Power (Sint16)
    // [15-16] Avg Power (Sint16)
    // [17-18] Elapsed Time (Uint16)

    let cursor = 0;
    const flags = value.getUint16(cursor, true);
    cursor += 2;

    // 1. Stroke Rate & Count
    const strokeRateRaw = value.getUint8(cursor++);
    const strokeCount = value.getUint16(cursor, true);
    cursor += 2;

    // 2. Avg Stroke Rate
    const avgStrokeRateRaw = value.getUint8(cursor++);

    // 3. Total Distance (Uint24 is tricky, read 16 then 8)
    const distLow = value.getUint16(cursor, true);
    const distHigh = value.getUint8(cursor + 2);
    const distanceTotal = distLow | (distHigh << 16);
    cursor += 3;

    // 4. Inst Pace
    const instPace = value.getUint16(cursor, true);
    cursor += 2;

    // 5. Avg Pace
    const avgPace = value.getUint16(cursor, true);
    cursor += 2;

    // 6. Inst Power
    const instPower = value.getInt16(cursor, true);
    cursor += 2;

    // 7. Avg Power
    const avgPower = value.getInt16(cursor, true);
    cursor += 2;

    // 8. Elapsed Time
    const elapsedTime = value.getUint16(cursor, true);
    cursor += 2;

    // Format for App State
    const metrics = {
      strokesTotal: strokeCount,
      distanceTotal: distanceTotal,
      strokesPerMinute: strokeRateRaw / 2.0, // FTMS sends 2x precision
      splitFormatted: instPace, // Seconds/500m
      power: instPower,
      durationTotalFormatted: this.formatTime(elapsedTime),
      // Computed or extra fields
      caloriesTotal: this.estimateCalories(distanceTotal),
    };

    this.updateState({ metrics });
  }

  formatTime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h > 0 ? h + ":" : ""}${m.toString().padStart(2, "0")}:${s.toString().padStart(2, "0")}`;
  }

  estimateCalories(distance) {
    // Basic approximation: ~60 kCal per km (very rough)
    return Math.round(distance * 0.06);
  }
}
