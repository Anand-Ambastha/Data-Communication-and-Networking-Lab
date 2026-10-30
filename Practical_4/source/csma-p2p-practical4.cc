
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main()
{
    // Create two P2P nodes
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices = p2p.Install(p2pNodes);

    // Create CSMA network with 4 nodes
    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(3);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices = csma.Install(csmaNodes);

    // Install Internet stack on all unique nodes
    NodeContainer allNodes;
    allNodes.Add(p2pNodes.Get(0));
    allNodes.Add(p2pNodes.Get(1));
    allNodes.Add(csmaNodes.Get(1));
    allNodes.Add(csmaNodes.Get(2));
    allNodes.Add(csmaNodes.Get(3));

    InternetStackHelper stack;
    stack.Install(allNodes);

    // Assign P2P IP addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(p2pDevices);

    // Assign CSMA IP addresses
    address.SetBase("10.1.2.0", "255.255.255.0");

    Ipv4InterfaceContainer csmaInterfaces =
        address.Assign(csmaDevices);

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // UDP Echo Server on Node 4
    UdpEchoServerHelper server(9);

    ApplicationContainer serverApp =
        server.Install(csmaNodes.Get(3));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // UDP Echo Client on Node 0
    UdpEchoClientHelper client(
        csmaInterfaces.GetAddress(3), 9);

    client.SetAttribute("MaxPackets", UintegerValue(5));
    client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    client.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp =
        client.Install(p2pNodes.Get(0));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // Generate PCAP files
    p2p.EnablePcapAll("second");
    csma.EnablePcap("second", csmaDevices.Get(0), true);

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    std::cout << "\n===== P2P AND CSMA RESULTS =====\n";
    std::cout << "P2P Bandwidth: 5 Mbps\n";
    std::cout << "CSMA Bandwidth: 100 Mbps\n";
    std::cout << "P2P Network: 10.1.1.0\n";
    std::cout << "CSMA Network: 10.1.2.0\n";
    std::cout << "UDP Packets: 5\n";
    std::cout << "Packet Size: 1024 bytes\n";
    std::cout << "================================\n";

    Simulator::Destroy();

    return 0;
}
