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
    uint32_t N = 100; // Number of mobile devices
    uint32_t K = 4;   // Number of tasks per device
    double serverCpu = 100.0; // CPU power of the MEC server in GHz
    Time simulationTime = Seconds(10); // Total simulation time

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Create mobile devices and server
    NodeContainer mobileDevices;
    mobileDevices.Create(N);
    NodeContainer server;
    server.Create(1);

    // Setup networking
    InternetStackHelper stack;
    stack.Install(mobileDevices);
    stack.Install(server);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer serverDevices = p2p.Install(mobileDevices.Get(0), server.Get(0)); // Example for simplicity

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(serverDevices);

    // Setup mobility
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

    // Energy model
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100));
    EnergySourceContainer sources = energySourceHelper.Install(mobileDevices);

    SimpleDeviceEnergyModelHelper energyModelHelper;
    energyModelHelper.Set("InitialEnergyJ", DoubleValue(100));
    DeviceEnergyModelContainer deviceModels = energyModelHelper.Install(serverDevices, sources);

    // Initialize random variables
    Ptr<UniformRandomVariable> dataSizeVar = CreateObject<UniformRandomVariable>();
    dataSizeVar->SetAttribute("Min", DoubleValue(0));
    dataSizeVar->SetAttribute("Max", DoubleValue(10)); // in MB
    Ptr<UniformRandomVariable> cpuPowerVar = CreateObject<UniformRandomVariable>();
    cpuPowerVar->SetAttribute("Min", DoubleValue(0.5));
    cpuPowerVar->SetAttribute("Max", DoubleValue(1.0)); // in GHz
    Ptr<UniformRandomVariable> energyPerCycleVar = CreateObject<UniformRandomVariable>();
    energyPerCycleVar->SetAttribute("Min", DoubleValue(0));
    energyPerCycleVar->SetAttribute("Max", DoubleValue(2e-10)); // in J/cycle

    // Application setup
    uint16_t port = 9; // Port number for UDP server
    UdpServerHelper serverApp(port);
    ApplicationContainer serverApps = serverApp.Install(server.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(simulationTime);

    UdpClientHelper clientApp(interfaces.GetAddress(1), port);
    clientApp.SetAttribute("MaxPackets", UintegerValue(1));
    clientApp.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    clientApp.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = clientApp.Install(mobileDevices.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(simulationTime);

    Simulator::Stop(simulationTime);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
