
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main()
{
    // Create WiFi station nodes and AP
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(3);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Configure WiFi channel and PHY
    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Configure WiFi and rate adaptation
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    wifi.SetRemoteStationManager(
        "ns3::MinstrelWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("NS3-WiFi");

    // Configure station MAC
    mac.SetType(
        "ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices =
        wifi.Install(phy, mac, wifiStaNodes);

    // Configure AP MAC
    mac.SetType(
        "ns3::ApWifiMac",
        "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevices =
        wifi.Install(phy, mac, wifiApNode);

    // Configure mobility
    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(10.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(
            Rectangle(-50, 50, -50, 50)));

    mobility.Install(wifiStaNodes);

    MobilityHelper apMobility;
    apMobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    apMobility.Install(wifiApNode);

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(wifiStaNodes);
    stack.Install(wifiApNode);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer staInterfaces =
        address.Assign(staDevices);

    Ipv4InterfaceContainer apInterface =
        address.Assign(apDevices);

    // UDP receiver at AP
    uint16_t port = 9;

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp =
        sink.Install(wifiApNode.Get(0));

    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(10.0));

    // UDP traffic from all stations
    for (uint32_t i = 0; i < 3; i++)
    {
        OnOffHelper source(
            "ns3::UdpSocketFactory",
            InetSocketAddress(
                apInterface.GetAddress(0), port));

        source.SetAttribute(
            "DataRate", StringValue("2Mbps"));

        source.SetAttribute(
            "PacketSize", UintegerValue(1024));

        ApplicationContainer app =
            source.Install(wifiStaNodes.Get(i));

        app.Start(Seconds(1.0));
        app.Stop(Seconds(9.0));
    }

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Display results
    Ptr<PacketSink> receiver =
        DynamicCast<PacketSink>(sinkApp.Get(0));

    std::cout << "\n===== WIFI RATE ADAPTATION RESULTS =====\n";

    std::cout << "WiFi Standard: IEEE 802.11g\n";
    std::cout << "Rate Adaptation: Minstrel\n";
    std::cout << "Number of Stations: 3\n";
    std::cout << "SSID: NS3-WiFi\n";

    std::cout << "Total Received Bytes: "
              << receiver->GetTotalRx() << std::endl;

    double throughput =
        receiver->GetTotalRx() * 8.0 / 10.0 / 1000000.0;

    std::cout << "Average Throughput: "
              << throughput << " Mbps\n";

    std::cout << "========================================\n";

    Simulator::Destroy();

    return 0;
}
