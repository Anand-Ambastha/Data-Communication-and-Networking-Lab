
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main()
{
    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Point-to-point link
    PointToPointHelper p2p;

    p2p.SetDeviceAttribute(
        "DataRate", StringValue("5Mbps"));

    p2p.SetChannelAttribute(
        "Delay", StringValue("2ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(devices);

    uint16_t port = 9;

    // UDP Server on Node 2
    UdpServerHelper server(port);

    ApplicationContainer serverApp =
        server.Install(nodes.Get(1));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // UDP Client on Node 1
    UdpClientHelper client(
        interfaces.GetAddress(1), port);

    client.SetAttribute(
        "MaxPackets", UintegerValue(10));

    client.SetAttribute(
        "Interval", TimeValue(Seconds(1.0)));

    client.SetAttribute(
        "PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp =
        client.Install(nodes.Get(0));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Display results
    Ptr<UdpServer> udpServer =
        DynamicCast<UdpServer>(serverApp.Get(0));

    std::cout << "\n===== UDP TRANSFER RESULTS =====\n";
    std::cout << "Client IP: " << interfaces.GetAddress(0) << "\n";
    std::cout << "Server IP: " << interfaces.GetAddress(1) << "\n";
    std::cout << "Packets Sent: 10\n";
    std::cout << "Packets Received: "
              << udpServer->GetReceived() << "\n";
    std::cout << "Packet Size: 1024 bytes\n";
    std::cout << "================================\n";

    Simulator::Destroy();

    return 0;
}
