#ifndef _CAN_INTERFACE_H_
#define _CAN_INTERFACE_H_

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

/**
 * Wire protocol for the CAN-to-Ethernet bridge.
 * Each frame on the wire is 13 bytes:
 *   [0]      header:  (data_len & 0x0F) | (extended ? 0x80 : 0x00)
 *   [1..4]   CAN ID   (big-endian uint32)
 *   [5..12]  data     (8 bytes, zero-padded)
 */
struct CANFrame
{
    uint32_t id;
    uint8_t data[8];
    uint32_t timestamp; // ms since epoch, filled on receipt

    CANFrame() : id(0), data{}, timestamp(0) {}
    CANFrame(uint32_t id, const uint8_t *d) : id(id), timestamp(0)
    {
        std::memcpy(data, d, 8);
    }

    std::string Str() const;
};

using FrameCallback = std::function<void(const CANFrame &)>;
using ConnectCallback = std::function<void()>;
using DisconnectCallback = std::function<void()>;

/**
 * TCP client that connects to a CAN-to-Ethernet bridge,
 * reads 13-byte wire frames, and dispatches decoded
 * CANFrames via callbacks. Mirrors the Java EthInterface.
 */
class CanInterface
{
public:
    CanInterface(const std::string &host, uint16_t port, bool verbose = false);
    ~CanInterface();

    CanInterface(const CanInterface &) = delete;
    CanInterface &operator=(const CanInterface &) = delete;

    void SetOnFrame(FrameCallback cb);
    void SetOnConnect(ConnectCallback cb);
    void SetOnDisconnect(DisconnectCallback cb);

    /** Blocking TCP connect with 3s timeout */
    bool Connect();
    void Disconnect();

    /** Connect and spawn the rx thread */
    void Start();
    void Stop();

    bool IsConnected() const;

    /** Pack and send a CAN frame over the wire */
    bool SendFrame(uint32_t can_id, const uint8_t *data, uint8_t len = 8);

private:
    /** Read loop: blocks on recv, decodes frames, fires on_frame_ */
    void RxLoop();

    /** Reads exactly len bytes from the socket, returns false on EOF/error */
    bool ReadFully(uint8_t *buf, size_t len);

    std::string host_;
    uint16_t port_;
    bool verbose_;
    int sockfd_{-1};

    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::thread rx_thread_;
    std::mutex write_mu_;

    FrameCallback on_frame_;
    ConnectCallback on_connect_;
    DisconnectCallback on_disconnect_;
};

#endif