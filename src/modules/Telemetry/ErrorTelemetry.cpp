#include "ErrorTelemetry.h"
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "Default.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "PowerFSM.h"
#include "RTC.h"
#include "RadioLibInterface.h"
#include "Router.h"
#include "configuration.h"
#include "main.h"
#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>
#include <meshUtils.h>

int32_t ErrorTelemetryModule::runOnce()
{
    refreshUptime();
    bool isImpoliteRole =
        IS_ONE_OF(config.device.role, meshtastic_Config_DeviceConfig_Role_SENSOR, meshtastic_Config_DeviceConfig_Role_ROUTER);
    if (((lastSentToMesh == 0) ||
         ((uptimeLastMs - lastSentToMesh) >=
          Default::getConfiguredOrDefaultMsScaled(moduleConfig.telemetry.device_update_interval,
                                                  default_telemetry_broadcast_interval_secs, numOnlineNodes))) &&
        airTime->isTxAllowedChannelUtil(!isImpoliteRole) && airTime->isTxAllowedAirUtil() &&
        config.device.role != meshtastic_Config_DeviceConfig_Role_REPEATER &&
        config.device.role != meshtastic_Config_DeviceConfig_Role_CLIENT_HIDDEN) {
        sendTelemetry();
        lastSentToMesh = uptimeLastMs;
    } else if (service->isToPhoneQueueEmpty()) {
        // Just send to phone when it's not our time to send to mesh yet
        // Only send while queue is empty (phone assumed connected)
        sendTelemetry(NODENUM_BROADCAST, true);
    }
    return sendToPhoneIntervalMs;
}

bool ErrorTelemetryModule::handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_Telemetry *t)
{
    // Don't worry about storing telemetry in NodeDB if we're a repeater
    if (config.device.role == meshtastic_Config_DeviceConfig_Role_REPEATER)
        return false;

    if (t->which_variant == meshtastic_Telemetry_device_metrics_tag) {
#ifdef DEBUG_PORT
        const char *sender = getSenderShortName(mp);

        LOG_INFO("(Received from %s): air_util_tx=%f, channel_utilization=%f, battery_level=%i, voltage=%f", sender,
                 t->variant.device_metrics.air_util_tx, t->variant.device_metrics.channel_utilization,
                 t->variant.device_metrics.battery_level, t->variant.device_metrics.voltage);
#endif
        nodeDB->updateTelemetry(getFrom(&mp), *t, RX_SRC_RADIO);
    }
    return false; // Let others look at this message also if they want
}

meshtastic_MeshPacket *ErrorTelemetryModule::allocReply()
{
    if (currentRequest) {
        auto req = *currentRequest;
        const auto &p = req.decoded;
        meshtastic_Telemetry scratch;
        meshtastic_Telemetry *decoded = NULL;
        memset(&scratch, 0, sizeof(scratch));
        if (pb_decode_from_bytes(p.payload.bytes, p.payload.size, &meshtastic_Telemetry_msg, &scratch)) {
            decoded = &scratch;
        } else {
            LOG_ERROR("Error decoding ErrorTelemetry module!");
            return NULL;
        }
        // Check for a request for error metrics
        if (decoded->which_variant == meshtastic_Telemetry_device_metrics_tag) {
            LOG_INFO("Error telemetry reply to request");
            return allocDataProtobuf(getErrorTelemetry());
        }
    }
    return NULL;
}

meshtastic_Telemetry ErrorTelemetryModule::getErrorTelemetry()
{
    // Total sensed packets (bad and good)
    this->lastSensedCount = this->sensedCount;
    this->sensedCount = RadioLibInterface::instance->rxBad + RadioLibInterface::instance->rxGood;
    this->sensedCount -= this->lastSensedCount;

    // Total transmit packets
    this->lastTransmitCount = this->transmitCount;
    this->transmitCount = RadioLibInterface::instance->txGood;
    this->transmitCount -= this->lastTransmitCount;

    // Total collided packets
    this->collisionCount = this->timingCollisionCount + RadioLibInterface::instance->rxBad;

    meshtastic_Telemetry t = meshtastic_Telemetry_init_zero;
    t.which_variant = meshtastic_Telemetry_error_metrics_tag;
    t.time = getTime();
    // Some time period which the measures occur over as set by users
    t.variant.error_metrics = meshtastic_ErrorMetrics_init_zero;
    t.variant.error_metrics.has_period = true;
    t.variant.error_metrics.has_collision_rate = true;
    t.variant.error_metrics.has_reachability = true;
    t.variant.error_metrics.has_usefulness = true;
    t.variant.error_metrics.has_avg_delay = true;
    // the following might be better done already in the grafana dashboard
    t.variant.error_metrics.has_avg_tx_air_util = true;

    t.variant.error_metrics.period = millis() - lastSentToMesh;

    // Increment collision count if a power, frequency, spreading factor, and timing collide
    // Then our collision rate is that count / the count of sensed packets
    t.variant.error_metrics.collision_rate = (float)this->collisionCount / (float)this->sensedCount;

    size_t numNodes = nodeDB->getNumMeshNodes();
    if (numNodes > 0)
        numNodes--;
    t.variant.error_metrics.reachability = (float)this->usefulCount / ((float)this->transmitCount * (float)numNodes);

    t.variant.error_metrics.usefulness = (float)this->usefulCount / (float)this->receivedCount;

    t.variant.error_metrics.avg_delay = this->avg_tx_delay;

    // the following might be better done already in the grafana dashboard
    t.variant.error_metrics.avg_tx_air_util = this->avg_tx_airutil;

    return t;
}

bool ErrorTelemetryModule::sendTelemetry(NodeNum dest, bool phoneOnly)
{
    meshtastic_Telemetry telemetry = getErrorTelemetry();
    LOG_INFO("Send: period=%f, collision_rate=%f, reachability=%f, usefulness=%f, avg_delay=%f, avg_tx_air_util=%f",
             telemetry.variant.error_metrics.period, telemetry.variant.error_metrics.collision_rate,
             telemetry.variant.error_metrics.reachability, telemetry.variant.error_metrics.avg_delay,
             telemetry.variant.error_metrics.avg_tx_air_util);

    meshtastic_MeshPacket *p = allocDataProtobuf(telemetry);
    p->to = dest;
    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;

    nodeDB->updateTelemetry(nodeDB->getNodeNum(), telemetry, RX_SRC_LOCAL);
    if (phoneOnly) {
        LOG_INFO("Send packet to phone");
        service->sendToPhone(p);
    } else {
        LOG_INFO("Send packet to mesh");
        this->lastSentToMesh = millis();
        service->sendToMesh(p, RX_SRC_LOCAL, true);
    }
    return true;
}
