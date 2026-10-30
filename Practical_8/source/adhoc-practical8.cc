
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdhocScriptExample");

int main()
{
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Configure WiFi channel
    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Configure 802.11b WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("DsssRate2Mbps"),
        "ControlMode", StringValue("DsssRate1Mbps"));

    // Ad hoc MAC configuration
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices =
        wifi.Install(phy, mac, nodes);

    // Node positions
    MobilityHelper mobility;

    Ptr<ListPositionAllocator> position =
        CreateObject<ListPositionAllocator>();

    position->Add(Vector(0, 0, 0));
    position->Add(Vector(10, 0, 0));

    mobility.SetPositionAllocator(position);

    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    mobility.Install(nodes);

    // Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(devices);

    // UDP Echo Server
    uint16_t port = 9;

    UdpEchoServerHelper server(port);

    ApplicationContainer serverApp =
        server.Install(nodes.Get(1));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // UDP Echo Client
    UdpEchoClientHelper client(
        interfaces.GetAddress(1), port);

    client.SetAttribute("MaxPackets", UintegerValue(5));
    client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    client.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp =
        client.Install(nodes.Get(0));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    std::cout << "\n===== AD HOC NETWORK RESULTS =====\n";
    std::cout << "WiFi Standard: 802.11b\n";
    std::cout << "Network IP: 10.1.1.0\n";
    std::cout << "Client IP: " << interfaces.GetAddress(0) << "\n";
    std::cout << "Server IP: " << interfaces.GetAddress(1) << "\n";
    std::cout << "Packet Size: 1024 bytes\n";
    std::cout << "Packets Sent: 5\n";
    std::cout << "==================================\n";

    Simulator::Destroy();

    return 0;
}
