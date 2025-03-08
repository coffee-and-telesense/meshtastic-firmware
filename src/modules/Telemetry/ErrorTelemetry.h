#pragma once
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "NodeDB.h"
#include "PhoneAPI.h"
#include "ProtobufModule.h"
#include "main.h"
#include "meshtastic/portnums.pb.h"
#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>
#include <cstdint>

class ErrorTelemetryModule : private concurrency::OSThread, public ProtobufModule<meshtastic_Telemetry>
{
    CallbackObserver<ErrorTelemetryModule, const meshtastic::Status *> nodeStatusObserver =
        CallbackObserver<ErrorTelemetryModule, const meshtastic::Status *>(this, &ErrorTelemetryModule::handleStatusUpdate);

  public:
    ErrorTelemetryModule()
        : concurrency::OSThread("ErrorTelemetry"),
          ProtobufModule("ErrorTelemetry", meshtastic_PortNum_TELEMETRY_APP, &meshtastic_Telemetry_msg)
    {
        uptimeWrapCount = 0;
        uptimeLastMs = millis();
        nodeStatusObserver.observe(&nodeStatus->onNewStatus);
        setIntervalFromNow(45 * 2000); // Warm up a bit
    }
    virtual bool wantUIFrame() { return false; }

    /*
      -Override the wantPacket method. We need the RxRSSI for counting sensed packets
      in order to calculate the collision rate
    */
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override
    {
        switch (p->decoded.portnum) {
        case meshtastic_PortNum_TELEMETRY_APP:
            return true;
        default:
            return false;
        }
    }

    uint32_t timingCollisionCount = 0;
    uint32_t count_avg_delay = 0;
    uint32_t avg_tx_delay = 0.0f;
    uint32_t receivedCount = 0;

  protected:
    /** Called to handle a particular incoming message
    @return true if you've guaranteed you've handled this message and no other handlers should be considered for it
    */
    virtual bool handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_Telemetry *p) override;
    virtual meshtastic_MeshPacket *allocReply() override;
    virtual int32_t runOnce() override;
    /**
     * Send our Telemetry into the mesh
     */
    bool sendTelemetry(NodeNum dest = NODENUM_BROADCAST, bool phoneOnly = false);

    /**
     * Get the uptime in seconds
     * Loses some accuracy after 49 days, but that's fine
     */
    uint32_t getUptimeSeconds() { return (0xFFFFFFFF / 1000) * uptimeWrapCount + (uptimeLastMs / 1000); }

  private:
    meshtastic_Telemetry getErrorTelemetry();

    uint32_t sendToPhoneIntervalMs = SECONDS_IN_MINUTE * 1000;           // Send to phone every minute
    uint32_t sendStatsToPhoneIntervalMs = 15 * SECONDS_IN_MINUTE * 1000; // Send stats to phone every 15 minutes
    uint32_t lastSentStatsToPhone = 0;
    uint32_t lastSentToMesh = 0;

    uint32_t usefulCount = 0;
    uint32_t lastUsefulCount = 0;
    uint32_t collisionCount = 0;
    uint32_t lastCollisionCount = 0;
    uint32_t sensedCount = 0;
    uint32_t lastSensedCount = 0;
    uint32_t lastReceivedCount = 0;
    uint32_t transmitCount = 0;
    uint32_t lastTransmitCount = 0;

    void refreshUptime()
    {
        auto now = millis();
        // If we wrapped around (~49 days), increment the wrap count
        if (now < uptimeLastMs)
            uptimeWrapCount++;

        uptimeLastMs = now;
    }

    uint32_t uptimeWrapCount;
    uint32_t uptimeLastMs;
};
