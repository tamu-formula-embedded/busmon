// busmon — CAN bus monitor with live terminal display
//
// Usage:  busmon -f mappings.txt [-h host] [-p port] [-r rate_hz] [-v]

#include "can_interface.h"
#include "display.h"
#include "packet_mapper.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <thread>

static constexpr const char *DEFAULT_HOST = "192.168.0.20";
static constexpr uint16_t DEFAULT_PORT = 10001;
static constexpr int DEFAULT_HZ = 10;

static std::atomic<bool> g_running{true};
static void SigHandler(int) { g_running.store(false); }

struct Args
{
    std::string config_path;
    std::string host = DEFAULT_HOST;
    uint16_t port = DEFAULT_PORT;
    int hz = DEFAULT_HZ;
    bool verbose = false;
};

static void PrintUsage(const char *argv0)
{
    std::fprintf(stderr,
                 "Usage: %s -f <mappings.txt> [options]\n"
                 "\n"
                 "Options:\n"
                 "  -f <path>    Packet mapping config file (required)\n"
                 "  -h <host>    CAN bridge host  [%s]\n"
                 "  -p <port>    CAN bridge port  [%u]\n"
                 "  -r <hz>      Display refresh rate  [%d]\n"
                 "  -v           Verbose CAN traffic logging\n",
                 argv0, DEFAULT_HOST, DEFAULT_PORT, DEFAULT_HZ);
}

static bool ParseArgs(int argc, char **argv, Args &args)
{
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            args.config_path = argv[++i];
        }
        else if (std::strcmp(argv[i], "-h") == 0 && i + 1 < argc)
        {
            args.host = argv[++i];
        }
        else if (std::strcmp(argv[i], "-p") == 0 && i + 1 < argc)
        {
            args.port = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else if (std::strcmp(argv[i], "-r") == 0 && i + 1 < argc)
        {
            args.hz = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "-v") == 0)
        {
            args.verbose = true;
        }
        else
        {
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return false;
        }
    }
    if (args.config_path.empty())
    {
        std::fprintf(stderr, "Error: -f <mappings.txt> is required\n\n");
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    Args args;
    if (!ParseArgs(argc, argv, args))
    {
        PrintUsage(argv[0]);
        return 1;
    }

    PacketMapper mapper;
    if (!mapper.LoadMappings(args.config_path))
    {
        std::cerr << "Failed to load mappings from " << args.config_path << "\n";
        return 1;
    }

    if (args.verbose)
        std::cout << "Loaded mappings:\n"
                  << mapper.Str() << "\n";

    DisplayState display;

    CanInterface can(args.host, args.port, args.verbose);

    can.SetOnFrame([&](const CANFrame &pkt)
                   {
        std::vector<MappedPacket> updated;
        mapper.MapPacket(pkt, updated);

        std::lock_guard<std::mutex> lock(display.mu);
        display.rx_count++;
        for (auto& mp : updated)
            display.values.insert_or_assign(mp.identifier, mp); });

    can.SetOnConnect([&]()
                     {
        std::lock_guard<std::mutex> lock(display.mu);
        display.connected = true; });

    can.SetOnDisconnect([&]()
                        {
        std::lock_guard<std::mutex> lock(display.mu);
        display.connected = false; });

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    can.Start();

    auto t0 = std::chrono::steady_clock::now();
    Term::HideCursor();
    Term::Clear();

    while (g_running.load())
    {
        uint64_t uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - t0)
                              .count();

        {
            std::lock_guard<std::mutex> lock(display.mu);
            RenderTable(display, mapper, uptime);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / args.hz));
    }

    Term::ShowCursor();
    Term::Reset();
    std::printf("\nbusmon: shutting down\n");

    can.Stop();
    return 0;
}