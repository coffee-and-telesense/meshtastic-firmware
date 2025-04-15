#include "ErrorTelemetry.h"
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "DebugConfiguration.h"
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
    moduleConfig.telemetry.error_measurement_enabled = 1;
    moduleConfig.telemetry.error_update_interval = 60;

    if (!(moduleConfig.telemetry.error_measurement_enabled)) {
        // If this module is not enabled, and the user doesn't want the display screen don't waste any OSThread time on it
        return disable();
    } else {
        LOG_INFO("Error metrics telemetry: init");
    }
    refreshUptime();
    bool isImpoliteRole =
        IS_ONE_OF(config.device.role, meshtastic_Config_DeviceConfig_Role_SENSOR, meshtastic_Config_DeviceConfig_Role_ROUTER);
    if (((lastSentToMesh == 0) ||
         ((uptimeLastMs - lastSentToMesh) >= Default::getConfiguredOrDefaultMsScaled(moduleConfig.telemetry.error_update_interval,
                                                                                     default_telemetry_broadcast_interval_secs,
                                                                                     numOnlineNodes))) &&
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

    if (t->which_variant == meshtastic_Telemetry_error_metrics_tag) {
#ifdef DEBUG_PORT
        const char *sender = getSenderShortName(mp);
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
        if (decoded->which_variant == meshtastic_Telemetry_error_metrics_tag) {
            LOG_INFO("Error telemetry reply to request");
            return allocDataProtobuf(getErrorTelemetry());
        }
    }
    return NULL;
}

meshtastic_Telemetry ErrorTelemetryModule::getErrorTelemetry()
{
    if (RadioLibInterface::instance) {
        // Total received packets (good and bad)
        LOG_DEBUG("Sensed & Received count = %zu rxBads + %zu rxGoods", RadioLibInterface::instance->rxBad,
                  RadioLibInterface::instance->rxGood);
        this->receivedCount = RadioLibInterface::instance->rxBad + RadioLibInterface::instance->rxGood;

        // Total sensed packets (good and bad)
        // Assuming that sensed packets are the same as packets the antenna actually picks up, this is true.
        // Need to double check my understanding with a antenna person.
        this->sensedCount = this->receivedCount;

        // Total collided packets
        LOG_DEBUG("Collision count = %zu timing collisions + %zu rxBads + %zu txRelayCancels", this->timingCollisionCount,
                  RadioLibInterface::instance->rxBad, router->txRelayCanceled);
        this->collisionCount = this->timingCollisionCount + RadioLibInterface::instance->rxBad + router->txRelayCanceled;

        // Useful count is the received packets - dupes - bads
        // TODO: problem is that rxBads are being used in many different contexts for packet receptions
        // so: distinguish types of bads, add method to count sensed signals that may not be packets(?) for sensedCount
        LOG_DEBUG("Useful count = %zu received - %zu rxDupes - %zu rxBads", this->receivedCount, router->rxDupe,
                  RadioLibInterface::instance->rxBad);
        this->usefulCount = this->receivedCount - router->rxDupe - RadioLibInterface::instance->rxBad;
    }

    meshtastic_Telemetry t = meshtastic_Telemetry_init_zero;
    t.which_variant = meshtastic_Telemetry_error_metrics_tag;
    t.time = getTime();
    t.variant.error_metrics = meshtastic_ErrorMetrics_init_zero;

    // Some time period (seconds) which the measures occur over as set by users
    t.variant.error_metrics.has_period = true;
    t.variant.error_metrics.period = (millis() - this->lastSentToMesh) / 1000;

    // Increment collision count if a power, frequency, spreading factor, and timing collide
    // Then our collision rate is that count / the count of sensed packets
    if (this->sensedCount != 0) {
        t.variant.error_metrics.has_collision_rate = true;
        LOG_DEBUG("Collision rate calc: (%.2f collisions / %.2f sensed) * 100.0f", (float)this->collisionCount,
                  (float)this->sensedCount);
        t.variant.error_metrics.collision_rate = ((float)this->collisionCount / (float)this->sensedCount) * 100.0f;
    } else {
        t.variant.error_metrics.has_collision_rate = false;
    }

    size_t numNodes = nodeDB->getNumMeshNodes();
    size_t numOnline = nodeDB->getNumOnlineMeshNodes(false);
    if (numNodes > 0)
        numNodes--;
    if (numOnline > 0)
        numOnline--;
    if (numNodes != 0) {
        t.variant.error_metrics.has_node_reach = true;
        LOG_DEBUG("Node reach = (%.2f online / %.2f total) * 100.0f", (float)numOnline, (float)numNodes);
        t.variant.error_metrics.node_reach = ((float)numOnline / (float)numNodes) * 100.0f;
    } else {
        t.variant.error_metrics.has_node_reach = false;
    }

    if (numNodes != 0) {
        t.variant.error_metrics.has_num_nodes = true;
        t.variant.error_metrics.num_nodes = numNodes;
    } else {
        t.variant.error_metrics.num_nodes = false;
    }

    if (this->receivedCount != 0) {
        t.variant.error_metrics.has_usefulness = true;
        LOG_DEBUG("Useful rate = (%.2f useful pkts / %.2f received pkts) * 100.0f", (float)this->usefulCount,
                  (float)this->receivedCount);
        t.variant.error_metrics.usefulness = ((float)this->usefulCount / (float)this->receivedCount) * 100.0f;
    } else {
        t.variant.error_metrics.has_usefulness = false;
    }

    if (this->count_avg_delay != 0) {
        t.variant.error_metrics.has_avg_delay = true;
        LOG_DEBUG("Avg delay = (%zu total delay ms / %zu total count of delays)", this->total_tx_delay, this->count_avg_delay);
        t.variant.error_metrics.avg_delay = (this->total_tx_delay / this->count_avg_delay);
    } else {
        // Send 0 ms to report no average delay
        t.variant.error_metrics.has_avg_delay = true;
        t.variant.error_metrics.avg_delay = 0;
    }

    return t;
}

bool ErrorTelemetryModule::sendTelemetry(NodeNum dest, bool phoneOnly)
{
    meshtastic_Telemetry telemetry = getErrorTelemetry();
    LOG_INFO("Send: period=%zus", telemetry.variant.error_metrics.period);
    if (telemetry.variant.error_metrics.has_collision_rate)
        LOG_INFO("      collision_rate=%.2f%%", telemetry.variant.error_metrics.collision_rate);
    if (telemetry.variant.error_metrics.has_node_reach)
        LOG_INFO("      node_reach=%.2f%%", telemetry.variant.error_metrics.node_reach);
    if (telemetry.variant.error_metrics.has_num_nodes)
        LOG_INFO("      num_nodes=%zu", telemetry.variant.error_metrics.num_nodes);
    if (telemetry.variant.error_metrics.has_usefulness)
        LOG_INFO("      usefulness=%.2f%%", telemetry.variant.error_metrics.usefulness);
    if (telemetry.variant.error_metrics.has_avg_delay)
        LOG_INFO("      avg_delay=%zums", telemetry.variant.error_metrics.avg_delay);

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
        service->sendToMesh(p, RX_SRC_LOCAL, true);
    }

    // Reset values
    this->total_tx_delay = 0;
    this->count_avg_delay = 0;
    this->timingCollisionCount = 0;
    return true;
}
