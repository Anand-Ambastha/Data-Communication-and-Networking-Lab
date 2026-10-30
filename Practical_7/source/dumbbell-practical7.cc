
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main()
{
    // Create 6 nodes
    NodeContainer clients, routers, servers;
    clients.Create(2);
    routers.Create(2);
    servers.Create(2);

    // Access links
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    // Bottleneck link
    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    // Connect clients to left router
    NetDeviceContainer c1 = access.Install(clients.Get(0), routers.Get(0));
    NetDeviceContainer c2 = access.Install(clients.Get(1), routers.Get(0));

    // Connect the two routers
    NetDeviceContainer bridge = bottleneck.Install(routers);

    // Connect right router to servers
    NetDeviceContainer s1 = access.Install(routers.Get(1), servers.Get(0));
    NetDeviceContainer s2 = access.Install(routers.Get(1), servers.Get(1));

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(clients);
    internet.Install(routers);
    internet.Install(servers);

    // Assign IP addresses
    Ipv4AddressHelper ip;

    ip.SetBase("10.1.1.0", "255.255.255.0");
    ip.Assign(c1);

    ip.SetBase("10.1.2.0", "255.255.255.0");
    ip.Assign(c2);

    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(bridge);

    ip.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer server1 = ip.Assign(s1);

    ip.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer server2 = ip.Assign(s2);

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP receiver on Server 1
    uint16_t tcpPort = 50000;

    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));

    ApplicationContainer tcpReceiver =
        tcpSink.Install(servers.Get(0));

    tcpReceiver.Start(Seconds(0));
    tcpReceiver.Stop(Seconds(40));

    // TCP sender: Client 1 -> Server 1
    BulkSendHelper tcpSource(
        "ns3::TcpSocketFactory",
        InetSocketAddress(server1.GetAddress(1), tcpPort));

    tcpSource.SetAttribute("MaxBytes", UintegerValue(0));
    tcpSource.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer tcpSender =
        tcpSource.Install(clients.Get(0));

    tcpSender.Start(Seconds(1));
    tcpSender.Stop(Seconds(40));

    // UDP receiver on Server 2
    uint16_t udpPort = 50001;

    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), udpPort));

    ApplicationContainer udpReceiver =
        udpSink.Install(servers.Get(1));

    udpReceiver.Start(Seconds(0));
    udpReceiver.Stop(Seconds(40));

    // UDP Rate 1: Half bottleneck capacity (5 Mbps)
    OnOffHelper udpSource1(
        "ns3::UdpSocketFactory",
        InetSocketAddress(server2.GetAddress(1), udpPort));

    udpSource1.SetAttribute("DataRate", StringValue("5Mbps"));
    udpSource1.SetAttribute("PacketSize", UintegerValue(1024));
    udpSource1.SetAttribute("OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSource1.SetAttribute("OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpApp1 =
        udpSource1.Install(clients.Get(1));

    udpApp1.Start(Seconds(20));
    udpApp1.Stop(Seconds(30));

    // UDP Rate 2: Full bottleneck capacity (10 Mbps)
    OnOffHelper udpSource2(
        "ns3::UdpSocketFactory",
        InetSocketAddress(server2.GetAddress(1), udpPort));

    udpSource2.SetAttribute("DataRate", StringValue("10Mbps"));
    udpSource2.SetAttribute("PacketSize", UintegerValue(1024));
    udpSource2.SetAttribute("OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSource2.SetAttribute("OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpApp2 =
        udpSource2.Install(clients.Get(1));

    udpApp2.Start(Seconds(30));
    udpApp2.Stop(Seconds(40));

    // Flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(40));
    Simulator::Run();

    monitor->CheckForLostPackets();

    std::cout << "\n===== DUMBBELL NETWORK RESULTS =====\n";

    Ptr<PacketSink> tcpRx =
        DynamicCast<PacketSink>(tcpReceiver.Get(0));

    Ptr<PacketSink> udpRx =
        DynamicCast<PacketSink>(udpReceiver.Get(0));

    std::cout << "\nTCP Received Bytes: "
              << tcpRx->GetTotalRx();

    std::cout << "\nUDP Received Bytes: "
              << udpRx->GetTotalRx();

    std::cout << "\nTCP Average Throughput: "
              << tcpRx->GetTotalRx() * 8.0 / 39.0 / 1e6
              << " Mbps";

    std::cout << "\nUDP Average Throughput: "
              << udpRx->GetTotalRx() * 8.0 / 20.0 / 1e6
              << " Mbps";

    std::cout << "\n====================================\n";

    monitor->SerializeToXmlFile(
        "/content/dumbbell-flowmon.xml", true, true);

    Simulator::Destroy();

    return 0;
}
