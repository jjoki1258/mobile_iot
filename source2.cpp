#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/energy-module.h"
#include "ns3/udp-client-server-helper.h"
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MobileIoTNetworkSimulation");

int main(int argc, char* argv[])
{
    uint32_t N = 100; 
    uint32_t K = 4;   
    double serverCpu = 100.0; 

    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer mobileDevices, server;
    mobileDevices.Create(N);
    server.Create(1);

    InternetStackHelper stack;
    stack.Install(mobileDevices);
    stack.Install(server);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer serverDevices = p2p.Install(mobileDevices, server);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(serverDevices);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(10),
        "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(mobileDevices);

    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100));
    EnergySourceContainer sources = energySourceHelper.Install(mobileDevices);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Install(serverDevices, sources);

    UdpServerHelper serverApp(9);
    ApplicationContainer serverApps = serverApp.Install(server.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    for (uint32_t i = 0; i < mobileDevices.GetN(); ++i)
    {
        UdpClientHelper clientApp(interfaces.GetAddress(1), 9);
        clientApp.SetAttribute("MaxPackets", UintegerValue(K));
        clientApp.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        clientApp.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApps = clientApp.Install(mobileDevices.Get(i));
        clientApps.Start(Seconds(2.0));
        clientApps.Stop(Seconds(10.0));
    }

    Simulator::Stop(Seconds(10));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
