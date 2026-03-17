#ifndef _CAN_INTERFACE_H_
#define _CAN_INTERFACE_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

// Each frame on the wire is 13 bytes:
//   [0]      header:  (data_len & 0x0F) | (extended ? 0x80 : 0x00)
//   [1..4]   CAN ID   (big-endian uint32)
//   [5..12]  data     (8 bytes, zero-padded)
struct CANFrame
{
    uint32_t id;
    uint8_t data[8];
    uint32_t timestamp; // ms since epoch (filled on receipt)

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

class CanInterface
{
public:
    CanInterface(const std::string &host, uint16_t port, bool verbose = false);
    ~CanInterface();

    // Non-copyable
    CanInterface(const CanInterface &) = delete;
    CanInterface &operator=(const CanInterface &) = delete;

    void SetOnFrame(FrameCallback cb);
    void SetOnConnect(ConnectCallback cb);
    void SetOnDisconnect(DisconnectCallback cb);

    bool Connect(); // blocking connect
    void Disconnect();
    void Start(); // connect + spawn rx thread
    void Stop();

    bool IsConnected() const;

    bool SendFrame(uint32_t can_id, const uint8_t *data, uint8_t len = 8);

private:
    void RxLoop();
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

#endif // _CAN_INTERFACE_H_