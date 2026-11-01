
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

using namespace ns3;

struct Results
{
    double tcpThroughput;
    double udpThroughput;
    double avgDelay;
    double jitter;
    uint32_t lostPackets;
};

Results RunScenario(std::string algorithm, double udpRate)
{
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

    // Internet stack
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

    if (algorithm == "RED")
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
    else if (algorithm == "CoDel")
    {
        tch.SetRootQueueDisc(
            "ns3::CoDelQueueDisc",
            "MaxSize", QueueSizeValue(QueueSize("100p")));
    }
    else
    {
        tch.SetRootQueueDisc(
            "ns3::FifoQueueDisc",
            "MaxSize", QueueSizeValue(QueueSize("100p")));
    }

    // Remove default queue disciplines and install selected algorithm
    tch.Uninstall(bridge);
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

    // UDP sender with variable offered load
    OnOffHelper udpSource(
        "ns3::UdpSocketFactory",
        InetSocketAddress(udpServerIP.GetAddress(1), udpPort));

    udpSource.SetAttribute(
        "DataRate",
        DataRateValue(DataRate(static_cast<uint64_t>(udpRate * 1000000))));

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

    // FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    monitor->CheckForLostPackets();

    // Throughput
    Ptr<PacketSink> tcpReceiver =
        DynamicCast<PacketSink>(tcpRx.Get(0));

    Ptr<PacketSink> udpReceiver =
        DynamicCast<PacketSink>(udpRx.Get(0));

    Results result{};

    result.tcpThroughput =
        tcpReceiver->GetTotalRx() * 8.0 / 18.0 / 1e6;

    result.udpThroughput =
        udpReceiver->GetTotalRx() * 8.0 / 18.0 / 1e6;

    // Aggregate FlowMonitor statistics
    double totalDelay = 0.0;
    double totalJitter = 0.0;
    uint64_t receivedPackets = 0;

    result.lostPackets = 0;

    auto stats = monitor->GetFlowStats();

    for (const auto& flow : stats)
    {
        result.lostPackets += flow.second.lostPackets;

        totalDelay += flow.second.delaySum.GetSeconds();
        totalJitter += flow.second.jitterSum.GetSeconds();
        receivedPackets += flow.second.rxPackets;
    }

    if (receivedPackets > 0)
    {
        result.avgDelay = totalDelay / receivedPackets * 1000.0;
        result.jitter = totalJitter / receivedPackets * 1000.0;
    }
    else
    {
        result.avgDelay = 0.0;
        result.jitter = 0.0;
    }

    // Save individual FlowMonitor output
    std::string filename =
        "/content/exp11-" + algorithm + "-" +
        std::to_string(static_cast<int>(udpRate)) + "Mbps.xml";

    monitor->SerializeToXmlFile(filename, true, true);

    Simulator::Destroy();

    return result;
}

int main()
{
    std::string algorithms[] = {"DropTail", "RED", "CoDel"};
    double rates[] = {4.0, 8.0, 12.0};

    std::ofstream csv("/content/exp11-results.csv");

    csv << "Algorithm,UDP_Load_Mbps,TCP_Throughput_Mbps,"
        << "UDP_Throughput_Mbps,Average_Delay_ms,"
        << "Average_Jitter_ms,Lost_Packets\n";

    std::cout << "\n============================================\n";
    std::cout << " EXPERIMENT 11: AQM PERFORMANCE COMPARISON\n";
    std::cout << "============================================\n";

    std::cout << std::fixed << std::setprecision(3);

    for (const auto& algorithm : algorithms)
    {
        for (double rate : rates)
        {
            std::cout << "\nRunning " << algorithm
                      << " at UDP load " << rate << " Mbps...\n";

            Results r = RunScenario(algorithm, rate);

            std::cout << "TCP Throughput: "
                      << r.tcpThroughput << " Mbps\n";

            std::cout << "UDP Throughput: "
                      << r.udpThroughput << " Mbps\n";

            std::cout << "Average Delay: "
                      << r.avgDelay << " ms\n";

            std::cout << "Average Jitter: "
                      << r.jitter << " ms\n";

            std::cout << "FlowMonitor Lost Packets: "
                      << r.lostPackets << "\n";

            csv << algorithm << ","
                << rate << ","
                << r.tcpThroughput << ","
                << r.udpThroughput << ","
                << r.avgDelay << ","
                << r.jitter << ","
                << r.lostPackets << "\n";
        }
    }

    csv.close();

    std::cout << "\n============================================\n";
    std::cout << "Simulation completed.\n";
    std::cout << "CSV: /content/exp11-results.csv\n";
    std::cout << "============================================\n";

    return 0;
}
