
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>

using namespace ns3;

struct Results
{
    double tcpThroughput;
    double udpThroughput;
    uint32_t lostPackets;
};

Results RunScenario(bool useRED)
{
    // Create nodes
    NodeContainer clients, routers, servers;
    clients.Create(2);
    routers.Create(2);
    servers.Create(2);

    // Access links
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer c1 =
        access.Install(clients.Get(0), routers.Get(0));

    NetDeviceContainer c2 =
        access.Install(clients.Get(1), routers.Get(0));

    // Bottleneck link
    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer bridge = bottleneck.Install(routers);

    NetDeviceContainer s1 =
        access.Install(routers.Get(1), servers.Get(0));

    NetDeviceContainer s2 =
        access.Install(routers.Get(1), servers.Get(1));

    // Install Internet stack
    NodeContainer allNodes;
    allNodes.Add(clients);
    allNodes.Add(routers);
    allNodes.Add(servers);

    InternetStackHelper internet;
    internet.Install(allNodes);

    // IP addressing
    Ipv4AddressHelper ip;

    ip.SetBase("10.1.1.0", "255.255.255.0");
    ip.Assign(c1);

    ip.SetBase("10.1.2.0", "255.255.255.0");
    ip.Assign(c2);

    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(bridge);

    ip.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer tcpServerIP = ip.Assign(s1);

    ip.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer udpServerIP = ip.Assign(s2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure queue discipline
    TrafficControlHelper tch;

    if (useRED)
    {
        tch.SetRootQueueDisc(
            "ns3::RedQueueDisc",
            "MinTh", DoubleValue(5),
            "MaxTh", DoubleValue(15),
            "MaxSize", QueueSizeValue(QueueSize("100p")),
            "LinkBandwidth", StringValue("10Mbps"),
            "LinkDelay", StringValue("10ms"),
            "MeanPktSize", UintegerValue(1024));
    }
    else
    {
        tch.SetRootQueueDisc(
            "ns3::FifoQueueDisc",
            "MaxSize", QueueSizeValue(QueueSize("100p")));
    }

    // Remove automatically installed default queue disciplines
    tch.Uninstall(bridge);

    // Install the selected queue discipline on both bottleneck devices
    tch.Install(bridge);

    // TCP receiver
    uint16_t tcpPort = 50000;

    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));

    ApplicationContainer tcpRx = tcpSink.Install(servers.Get(0));
    tcpRx.Start(Seconds(0));
    tcpRx.Stop(Seconds(20));

    // TCP sender
    BulkSendHelper tcpSource(
        "ns3::TcpSocketFactory",
        InetSocketAddress(tcpServerIP.GetAddress(1), tcpPort));

    tcpSource.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer tcpTx = tcpSource.Install(clients.Get(0));
    tcpTx.Start(Seconds(1));
    tcpTx.Stop(Seconds(19));

    // UDP receiver
    uint16_t udpPort = 50001;

    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), udpPort));

    ApplicationContainer udpRx = udpSink.Install(servers.Get(1));
    udpRx.Start(Seconds(0));
    udpRx.Stop(Seconds(20));

    // UDP sender
    OnOffHelper udpSource(
        "ns3::UdpSocketFactory",
        InetSocketAddress(udpServerIP.GetAddress(1), udpPort));

    udpSource.SetAttribute("DataRate", StringValue("8Mbps"));
    udpSource.SetAttribute("PacketSize", UintegerValue(1024));

    udpSource.SetAttribute(
        "OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));

    udpSource.SetAttribute(
        "OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpTx = udpSource.Install(clients.Get(1));
    udpTx.Start(Seconds(1));
    udpTx.Stop(Seconds(19));

    // Flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    monitor->CheckForLostPackets();

    // Collect results
    Ptr<PacketSink> tcpReceiver =
        DynamicCast<PacketSink>(tcpRx.Get(0));

    Ptr<PacketSink> udpReceiver =
        DynamicCast<PacketSink>(udpRx.Get(0));

    Results result;

    result.tcpThroughput =
        tcpReceiver->GetTotalRx() * 8.0 / 18.0 / 1e6;

    result.udpThroughput =
        udpReceiver->GetTotalRx() * 8.0 / 18.0 / 1e6;

    result.lostPackets = 0;

    auto stats = monitor->GetFlowStats();

    for (const auto& flow : stats)
    {
        result.lostPackets += flow.second.lostPackets;
    }

    // Save FlowMonitor output
    monitor->SerializeToXmlFile(
        useRED ? "/content/red-results.xml"
               : "/content/droptail-results.xml",
        true, true);

    Simulator::Destroy();

    return result;
}

int main()
{
    std::cout << "\n====================================\n";
    std::cout << " TCP UDP NETWORK PERFORMANCE PROJECT\n";
    std::cout << "====================================\n";

    // Scenario 1: DropTail
    std::cout << "\nRunning DropTail Simulation...\n";
    Results dropTail = RunScenario(false);

    // Scenario 2: RED
    std::cout << "\nRunning RED Simulation...\n";
    Results red = RunScenario(true);

    // Comparison
    std::cout << "\n========== FINAL COMPARISON ==========\n";

    std::cout << std::fixed << std::setprecision(3);

    std::cout << "\nDROP TAIL:\n";
    std::cout << "TCP Throughput: "
              << dropTail.tcpThroughput << " Mbps\n";
    std::cout << "UDP Throughput: "
              << dropTail.udpThroughput << " Mbps\n";
    std::cout << "FlowMonitor Lost Packets: "
              << dropTail.lostPackets << "\n";

    std::cout << "\nRED:\n";
    std::cout << "TCP Throughput: "
              << red.tcpThroughput << " Mbps\n";
    std::cout << "UDP Throughput: "
              << red.udpThroughput << " Mbps\n";
    std::cout << "FlowMonitor Lost Packets: "
              << red.lostPackets << "\n";

    std::cout << "\n======================================\n";

    return 0;
}
