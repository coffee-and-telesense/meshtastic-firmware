#include "SimRadio.h"
#include "MeshService.h"
#include "Router.h"
#if !MESHTASTIC_EXCLUDE_ERROR_TELEMETRY
#include "modules/Telemetry/ErrorTelemetry.h"
#endif
SimRadio::SimRadio() : NotifiedWorkerThread("SimRadio")
{
    instance = this;
}

SimRadio *SimRadio::instance;

ErrorCode SimRadio::send(meshtastic_MeshPacket *p)
{
    printPacket("enqueuing for send", p);

    ErrorCode res = txQueue.enqueue(p) ? ERRNO_OK : ERRNO_UNKNOWN;

    if (res != ERRNO_OK) { // we weren't able to queue it, so we must drop it to prevent leaks
        packetPool.release(p);
        return res;
    }

    // set (random) transmit delay to let others reconfigure their radio,
    // to avoid collisions and implement timing-based flooding
    LOG_DEBUG("Set random delay before tx");
    setTransmitDelay();
    return res;
}

void SimRadio::setTransmitDelay()
{
    meshtastic_MeshPacket *p = txQueue.getFront();
    // We want all sending/receiving to be done by our daemon thread.
    // We use a delay here because this packet might have been sent in response to a packet we just received.
    // So we want to make sure the other side has had a chance to reconfigure its radio.

    /* We assume if rx_snr = 0 and rx_rssi = 0, the packet was generated locally.
     *   This assumption is valid because of the offset generated by the radio to account for the noise
     *   floor.
     */
    if (p->rx_snr == 0 && p->rx_rssi == 0) {
        startTransmitTimer(true);
    } else {
        // If there is a SNR, start a timer scaled based on that SNR.
        LOG_DEBUG("rx_snr found. hop_limit:%d rx_snr:%f", p->hop_limit, p->rx_snr);
        startTransmitTimerSNR(p->rx_snr);
    }
}

void SimRadio::startTransmitTimer(bool withDelay)
{
    // If we have work to do and the timer wasn't already scheduled, schedule it now
    if (!txQueue.empty()) {
        uint32_t delayMsec = !withDelay ? 1 : getTxDelayMsec();
        // LOG_DEBUG("xmit timer %d", delay);
        notifyLater(delayMsec, TRANSMIT_DELAY_COMPLETED, false);
    }
}

void SimRadio::startTransmitTimerSNR(float snr)
{
    // If we have work to do and the timer wasn't already scheduled, schedule it now
    if (!txQueue.empty()) {
        uint32_t delayMsec = getTxDelayMsecWeighted(snr);
        // LOG_DEBUG("xmit timer %d", delay);
        notifyLater(delayMsec, TRANSMIT_DELAY_COMPLETED, false);
    }
}

void SimRadio::handleTransmitInterrupt()
{
    // This can be null if we forced the device to enter standby mode.  In that case
    // ignore the transmit interrupt
    if (sendingPacket)
        completeSending();

    isReceiving = true;
    if (receivingPacket) // This happens when we don't consider something a collision if we weren't sending long enough
        handleReceiveInterrupt();
}

void SimRadio::completeSending()
{
    // We are careful to clear sending packet before calling printPacket because
    // that can take a long time
    auto p = sendingPacket;
    sendingPacket = NULL;

    if (p) {
        txGood++;
        if (!isFromUs(p))
            txRelay++;
        printPacket("Completed sending", p);

        // We are done sending that packet, release it
        packetPool.release(p);
        // LOG_DEBUG("Done with send");
    }
}

/** Could we send right now (i.e. either not actively receiving or transmitting)? */
bool SimRadio::canSendImmediately()
{
    // We wait _if_ we are partially though receiving a packet (rather than just merely waiting for one).
    // To do otherwise would be doubly bad because not only would we drop the packet that was on the way in,
    // we almost certainly guarantee no one outside will like the packet we are sending.
    bool busyTx = sendingPacket != NULL;
    bool busyRx = isReceiving && isActivelyReceiving();

    if (busyTx || busyRx) {
        if (busyTx)
            LOG_WARN("Can not send yet, busyTx");
        if (busyRx)
            LOG_WARN("Can not send yet, busyRx");
        return false;
    } else
        return true;
}

bool SimRadio::isActivelyReceiving()
{
    return receivingPacket != nullptr;
}

bool SimRadio::isChannelActive()
{
    return receivingPacket != nullptr;
}

/** Attempt to cancel a previously sent packet.  Returns true if a packet was found we could cancel */
bool SimRadio::cancelSending(NodeNum from, PacketId id)
{
    auto p = txQueue.remove(from, id);
    if (p)
        packetPool.release(p); // free the packet we just removed

    bool result = (p != NULL);
    LOG_DEBUG("cancelSending id=0x%x, removed=%d", id, result);
    return result;
}

void SimRadio::onNotify(uint32_t notification)
{
    switch (notification) {
    case ISR_TX:
        handleTransmitInterrupt();
        //  LOG_DEBUG("tx complete - starting timer");
        startTransmitTimer();
        break;
    case ISR_RX:
        handleReceiveInterrupt();
        //  LOG_DEBUG("rx complete - starting timer");
        startTransmitTimer();
        break;
    case TRANSMIT_DELAY_COMPLETED:
        if (receivingPacket) { // This happens when we had a timer pending and we started receiving
            handleReceiveInterrupt();
            startTransmitTimer();
            break;
        }
        LOG_DEBUG("delay done");

        // If we are not currently in receive mode, then restart the random delay (this can happen if the main thread
        // has placed the unit into standby)  FIXME, how will this work if the chipset is in sleep mode?
        if (!txQueue.empty()) {
            if (!canSendImmediately()) {
                // LOG_DEBUG("Currently Rx/Tx-ing: set random delay");
                setTransmitDelay(); // currently Rx/Tx-ing: reset random delay
            } else {
                if (isChannelActive()) { // check if there is currently a LoRa packet on the channel
                    // LOG_DEBUG("Channel is active: set random delay");
                    setTransmitDelay(); // reset random delay
                } else {
                    // Send any outgoing packets we have ready
                    meshtastic_MeshPacket *txp = txQueue.dequeue();
                    assert(txp);
                    startSend(txp);
                    // Packet has been sent, count it toward our TX airtime utilization.
                    uint32_t xmitMsec = getPacketTime(txp);
                    airTime->logAirtime(TX_LOG, xmitMsec);

                    notifyLater(xmitMsec, ISR_TX, false); // Model the time it is busy sending
                }
            }
        } else {
            // LOG_DEBUG("done with txqueue");
        }
        break;
    default:
        assert(0); // We expected to receive a valid notification from the ISR
    }
}

/** start an immediate transmit */
void SimRadio::startSend(meshtastic_MeshPacket *txp)
{
    printPacket("Start low level send", txp);
    isReceiving = false;
    size_t numbytes = beginSending(txp);
    meshtastic_MeshPacket *p = packetPool.allocCopy(*txp);
    perhapsDecode(p);
    meshtastic_Compressed c = meshtastic_Compressed_init_default;
    c.portnum = p->decoded.portnum;
    // LOG_DEBUG("Send back to simulator with portNum %d", p->decoded.portnum);
    if (p->decoded.payload.size <= sizeof(c.data.bytes)) {
        memcpy(&c.data.bytes, p->decoded.payload.bytes, p->decoded.payload.size);
        c.data.size = p->decoded.payload.size;
    } else {
        LOG_WARN("Payload size larger than compressed message allows! Send empty payload");
    }
    p->decoded.payload.size =
        pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes), &meshtastic_Compressed_msg, &c);
    p->decoded.portnum = meshtastic_PortNum_SIMULATOR_APP;

    service->sendQueueStatusToPhone(router->getQueueStatus(), 0, p->id);
    service->sendToPhone(p); // Sending back to simulator
    service->loop();         // Process the send immediately
}

// Simulates device received a packet via the LoRa chip
void SimRadio::unpackAndReceive(meshtastic_MeshPacket &p)
{
    // Simulator packet (=Compressed packet) is encapsulated in a MeshPacket, so need to unwrap first
    meshtastic_Compressed scratch;
    meshtastic_Compressed *decoded = NULL;
    if (p.which_payload_variant == meshtastic_MeshPacket_decoded_tag) {
        memset(&scratch, 0, sizeof(scratch));
        p.decoded.payload.size =
            pb_decode_from_bytes(p.decoded.payload.bytes, p.decoded.payload.size, &meshtastic_Compressed_msg, &scratch);
        if (p.decoded.payload.size) {
            decoded = &scratch;
            // Extract the original payload and replace
            memcpy(&p.decoded.payload, &decoded->data, sizeof(decoded->data));
            // Switch the port from PortNum_SIMULATOR_APP back to the original PortNum
            p.decoded.portnum = decoded->portnum;
        } else
            LOG_ERROR("Error decoding proto for simulator message!");
    }
    // Let SimRadio receive as if it did via its LoRa chip
    startReceive(&p);
}

void SimRadio::startReceive(meshtastic_MeshPacket *p)
{
#ifdef USERPREFS_SIMRADIO_EMULATE_COLLISIONS
    if (isActivelyReceiving()) {
        LOG_WARN("Collision detected, dropping current and previous packet!");
        rxBad++;
        airTime->logAirtime(RX_ALL_LOG, getPacketTime(receivingPacket));
        packetPool.release(receivingPacket);
        receivingPacket = nullptr;
        return;
    } else if (sendingPacket) {
        uint32_t airtimeLeft = tillRun(millis());
        if (airtimeLeft <= 0) {
            LOG_WARN("Transmitting packet was already done");
            handleTransmitInterrupt(); // Finish sending first
        } else if ((interval - airtimeLeft) > preambleTimeMsec) {
            // Only if transmitting for longer than preamble there is a collision
            // (channel should actually be detected as active otherwise)
            LOG_WARN("Collision detected during transmission!");
            return;
        }
    }
    isReceiving = true;
    receivingPacket = packetPool.allocCopy(*p);
    uint32_t airtimeMsec = getPacketTime(p);
    notifyLater(airtimeMsec, ISR_RX, false); // Model the time it is busy receiving
#else
#if !MESHTASTIC_EXCLUDE_ERROR_TELEMETRY
    if (isActivelyReceiving()) {
        rxBad++;
        packetPool.release(receivingPacket);
        receivingPacket = nullptr;
        return;
    } else if (sendingPacket) {
        uint32_t airtimeLeft = tillRun(millis());
        if (airtimeLeft <= 0) {
            handleTransmitInterrupt(); // Finish sending first
        } else if ((interval - airtimeLeft) > preambleTimeMsec) {
            // Timing collision
            ErrorTelemetryModule->timingCollisionCount++;
            return;
        }
    }
#endif
    isReceiving = true;
    receivingPacket = packetPool.allocCopy(*p);
    handleReceiveInterrupt(); // Simulate receiving the packet immediately
    startTransmitTimer();
#endif
}

meshtastic_QueueStatus SimRadio::getQueueStatus()
{
    meshtastic_QueueStatus qs;

    qs.res = qs.mesh_packet_id = 0;
    qs.free = txQueue.getFree();
    qs.maxlen = txQueue.getMaxLen();

    return qs;
}

void SimRadio::handleReceiveInterrupt()
{
    if (receivingPacket == nullptr) {
        return;
    }

    if (!isReceiving) {
        LOG_DEBUG("*** WAS_ASSERT *** handleReceiveInterrupt called when not in receive mode");
        return;
    }

    LOG_DEBUG("HANDLE RECEIVE INTERRUPT");
    rxGood++;

    meshtastic_MeshPacket *mp = packetPool.allocCopy(*receivingPacket); // keep a copy in packetPool
    packetPool.release(receivingPacket);                                // release the original
    receivingPacket = nullptr;

    printPacket("Lora RX", mp);

    airTime->logAirtime(RX_LOG, getPacketTime(mp));

    deliverToReceiver(mp);
}

size_t SimRadio::getPacketLength(meshtastic_MeshPacket *mp)
{
    auto &p = mp->decoded;
    return (size_t)p.payload.size + sizeof(PacketHeader);
}

int16_t SimRadio::readData(uint8_t *data, size_t len)
{
    int16_t state = RADIOLIB_ERR_NONE;

    if (state == RADIOLIB_ERR_NONE) {
        // add null terminator
        data[len] = 0;
    }

    return state;
}
