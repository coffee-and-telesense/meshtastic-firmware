#pragma once
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "NodeDB.h"
#include "ProtobufModule.h"
#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>

class ErrorTelemetryModule : private concurrency::OSThread, public ProtobufModule<meshtastic_Telemetry>
{
    CallbackObserver<ErrorTelemetryModule, const meshtastic::Status *> nodeStatusObserver =
        CallbackObserver<ErrorTelemetryModule, const meshtastic::Status *>(this, &ErrorTelemetryModule::handleStatusUpdate);

  public:
    ErrorTelemetryModule()
        : concurrency::OSThread("DeviceTelemetry"),
          ProtobufModule("DeviceTelemetry", meshtastic_PortNum_TELEMETRY_APP, &meshtastic_Telemetry_msg)
    {
        uptimeWrapCount = 0;
        uptimeLastMs = millis();
        nodeStatusObserver.observe(&nodeStatus->onNewStatus);
        setIntervalFromNow(45 * 1000); // Wait until NodeInfo is sent
    }
    virtual bool wantUIFrame() { return false; }

    /*
      -Override the wantPacket method. We need the RxRSSI for counting sensed packets
      in order to calculate the collision rate
    */
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override
    {
        // TODO:
        //  need frequencies to check for freq collision for frequency collision in collision rate
        //  need speading factor to check if new packet == last packet sf for sf collision in collision rate
        //  need received time `millis()` to check for timing collision
        //  need number of preamble symbols to calculate timing collision
        //  need minimum received sensitivity to count sensed nodes for # collisions / # sensed
        //  need delay in ms
        //  need the tx air util for average tx air util
        //  need the seen sequence numbers to check if this is unseen and count useful

        // maybe count routing error types and report on each of them?
        if (p->rx_rssi != 0) {
            this->lastRxRssi = p->rx_rssi;
        }

        if (p->rx_snr > 0) {
            this->lastRxSnr = p->rx_snr;
        }

        // Get the average delay and add it to a running mean

        // Get the tx util and add it to a running mean

        // Increment useful count if we have not seen this sequence number before

        // Increment sensed packet count if the received rssi > minimum sensitivity

        // Increment collision count if a power, frequency, spreading factor, and timing collide
        // Then our collision rate is that count / the count of sensed packets

        // Return false since we aren't doing anything special, just counting.
        return false;
    }

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
