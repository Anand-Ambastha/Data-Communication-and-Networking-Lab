
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

uint32_t droppedPackets = 0;

void QueueTrace(uint32_t oldValue, uint32_t newValue)
{
    std::cout << "RED Queue: " << oldValue
              << " -> " << newValue << " packets" << std::endl;
}

void DropTrace(Ptr<const QueueDiscItem> item)
{
    droppedPackets++;
    std::cout << "RED: Packet dropped. Total drops = "
              << droppedPackets << std::endl;
}

int main(int argc, char *argv[])
{
    double simulationTime = 10.0;

    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Point-to-point network
    PointToPointHelper pointToPoint;

    pointToPoint.SetDeviceAttribute(
        "DataRate", StringValue("1Mbps"));

    pointToPoint.SetChannelAttribute(
        "Delay", StringValue("10ms"));

    // Small device queue
    pointToPoint.SetQueue(
        "ns3::DropTailQueue",
        "MaxSize", QueueSizeValue(QueueSize("1p")));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Configure RED queue discipline
    TrafficControlHelper tch;

    tch.SetRootQueueDisc(
        "ns3::RedQueueDisc",
        "MinTh", DoubleValue(5),
        "MaxTh", DoubleValue(15),
        "MaxSize", QueueSizeValue(QueueSize("10000p")),
        "LinkBandwidth", StringValue("1Mbps"),
        "LinkDelay", StringValue("10ms"),
        "MeanPktSize", UintegerValue(1024));

    // Install RED on sender's outgoing device
    NetDeviceContainer senderDevice;
    senderDevice.Add(devices.Get(0));

    QueueDiscContainer qdiscs = tch.Install(senderDevice);

    Ptr<QueueDisc> redQueue = qdiscs.Get(0);

    // Trace queue length and packet drops
    redQueue->TraceConnectWithoutContext(
        "PacketsInQueue",
        MakeCallback(&QueueTrace));

    redQueue->TraceConnectWithoutContext(
        "Drop",
        MakeCallback(&DropTrace));

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(devices);

    // UDP packet receiver
    uint16_t port = 9;

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp =
        sink.Install(nodes.Get(1));

    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    // UDP traffic generator
    OnOffHelper source(
        "ns3::UdpSocketFactory",
        InetSocketAddress(interfaces.GetAddress(1), port));

    source.SetAttribute(
        "DataRate", StringValue("5Mbps"));

    source.SetAttribute(
        "PacketSize", UintegerValue(1024));

    source.SetAttribute(
        "OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));

    source.SetAttribute(
        "OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer sourceApp =
        source.Install(nodes.Get(0));

    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(simulationTime));

    // Flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Calculate statistics
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    auto stats = monitor->GetFlowStats();

    std::cout << "\n========== RED SIMULATION RESULTS ==========\n";

    for (auto flow : stats)
    {
        auto t = classifier->FindFlow(flow.first);

        std::cout << "\nFlow ID: " << flow.first;
        std::cout << "\nSource: " << t.sourceAddress;
        std::cout << "\nDestination: " << t.destinationAddress;

        double throughput =
            (flow.second.rxBytes * 8.0) /
            (simulationTime * 1000000.0);

        std::cout << "\nThroughput: "
                  << throughput << " Mbps";

        std::cout << "\nPackets transmitted: "
                  << flow.second.txPackets;

        std::cout << "\nPackets received: "
                  << flow.second.rxPackets;

        std::cout << "\nPackets lost: "
                  << flow.second.lostPackets;

        std::cout << "\n";
    }

    std::cout << "\nTotal RED dropped packets: "
              << droppedPackets << std::endl;

    std::cout << "============================================\n";

    Simulator::Destroy();

    return 0;
}
