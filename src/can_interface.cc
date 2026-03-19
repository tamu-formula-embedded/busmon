#include "can_interface.h"
#include "utils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>

std::string CANFrame::Str() const
{
    std::string s = Utils::StrFmt("CANFrame{ id=0x%04X data=", id);
    for (int i = 0; i < 8; i++) s += Utils::StrFmt("%02x ", data[i]);
    s += Utils::StrFmt("ts=%u }", timestamp);
    return s;
}

CanInterface::CanInterface(const std::string& host, uint16_t port, bool verbose)
    : host_(host), port_(port), verbose_(verbose)
{
}

CanInterface::~CanInterface()
{
    Stop();
}

void CanInterface::SetOnFrame(FrameCallback cb)
{
    on_frame_ = std::move(cb);
}
void CanInterface::SetOnConnect(ConnectCallback cb)
{
    on_connect_ = std::move(cb);
}
void CanInterface::SetOnDisconnect(DisconnectCallback cb)
{
    on_disconnect_ = std::move(cb);
}

bool CanInterface::IsConnected() const
{
    return connected_.load();
}

bool CanInterface::Connect()
{
    if (connected_.load()) return true;

    struct addrinfo hints
    {
    }, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port_);
    int         rc       = getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &res);
    if (rc != 0 || !res)
    {
        if (verbose_) fprintf(stderr, "[can] resolve failed: %s\n", gai_strerror(rc));
        return false;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0)
    {
        if (verbose_) fprintf(stderr, "[can] socket(): %s\n", strerror(errno));
        freeaddrinfo(res);
        return false;
    }

    /* TCP_NODELAY + SO_KEEPALIVE to match the Java side */
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));

    /* 3s connect timeout */
    struct timeval tv
    {
        3, 0
    };
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        if (verbose_)
            fprintf(stderr, "[can] connect to %s:%u failed: %s\n", host_.c_str(), port_,
                    strerror(errno));
        ::close(fd);
        freeaddrinfo(res);
        return false;
    }
    freeaddrinfo(res);

    sockfd_ = fd;
    connected_.store(true);

    if (verbose_) printf("[can] connected to %s:%u\n", host_.c_str(), port_);

    if (on_connect_) on_connect_();
    return true;
}

void CanInterface::Disconnect()
{
    bool was = connected_.exchange(false);
    if (sockfd_ >= 0)
    {
        ::shutdown(sockfd_, SHUT_RDWR);
        ::close(sockfd_);
        sockfd_ = -1;
    }
    if (was && on_disconnect_) on_disconnect_();
}

void CanInterface::Start()
{
    if (running_.load()) return;
    running_.store(true);

    if (!Connect())
    {
        running_.store(false);
        return;
    }

    rx_thread_ = std::thread([this]() { RxLoop(); });
}

void CanInterface::Stop()
{
    running_.store(false);
    Disconnect();
    if (rx_thread_.joinable()) rx_thread_.join();
}

bool CanInterface::SendFrame(uint32_t can_id, const uint8_t* data, uint8_t len)
{
    if (!connected_.load()) return false;

    uint8_t pkt[13]{};
    uint8_t ext = (can_id > 0x7FF) ? 0x80 : 0x00;
    pkt[0]      = (len & 0x0F) | ext;

    /* CAN ID, big-endian */
    pkt[1] = (can_id >> 24) & 0xFF;
    pkt[2] = (can_id >> 16) & 0xFF;
    pkt[3] = (can_id >> 8) & 0xFF;
    pkt[4] = (can_id >> 0) & 0xFF;

    std::memcpy(pkt + 5, data, std::min<int>(len, 8));

    std::lock_guard<std::mutex> lock(write_mu_);
    ssize_t                     n = ::send(sockfd_, pkt, 13, MSG_NOSIGNAL);
    if (n != 13)
    {
        if (verbose_) fprintf(stderr, "[can] send failed: %s\n", strerror(errno));
        Disconnect();
        return false;
    }
    return true;
}

bool CanInterface::ReadFully(uint8_t* buf, size_t len)
{
    size_t off = 0;
    while (off < len)
    {
        ssize_t n = ::recv(sockfd_, buf + off, len - off, 0);
        if (n <= 0) return false;
        off += n;
    }
    return true;
}

void CanInterface::RxLoop()
{
    uint8_t buf[13];

    while (running_.load() && connected_.load())
    {
        if (!ReadFully(buf, 13))
        {
            if (running_.load() && verbose_) fprintf(stderr, "[can] rx: connection lost\n");
            break;
        }

        /* Decode -- mirrors Java EthInterface.handle() */
        uint32_t can_id = ((uint32_t)buf[1] << 24) | ((uint32_t)buf[2] << 16) |
                          ((uint32_t)buf[3] << 8) | ((uint32_t)buf[4]);

        uint8_t data[8];
        std::memcpy(data, buf + 5, 8);

        CANFrame frame(can_id, data);
        auto     now    = std::chrono::steady_clock::now().time_since_epoch();
        frame.timestamp = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now).count());

        if (verbose_) printf("[can] rx %s\n", frame.Str().c_str());

        if (on_frame_) on_frame_(frame);
    }

    Disconnect();
}