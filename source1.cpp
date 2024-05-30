#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MobileIoTNetworkSimulation");

int main(int argc, char* argv[])
{
    uint32_t N = 100; // 모바일 기기 수
    uint32_t K = 4;   // 기기 당 작업 수
    double serverCpu = 100.0; // GHz, 서버 CPU

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // 노드 생성
    NodeContainer mobileDevices;
    mobileDevices.Create(N);
    NodeContainer server;
    server.Create(1);

    // 인터넷 스택
    InternetStackHelper stack;
    stack.Install(mobileDevices);
    stack.Install(server);

    // IP 주소 할당
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(mobileDevices);
    Ipv4InterfaceContainer serverInterface = address.Assign(server);
    
    // Communication Model
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(mobileDevices.Get(0), server.Get(0)); // Example for one device
    
    // 이동성 설정
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
        "X", StringValue("100.0"),
        "Y", StringValue("100.0"),
        "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Mode", StringValue("Time"),
        "Time", StringValue("2s"),
        "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
        "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)));
    mobility.Install(mobileDevices);

    // Energy Model
    BasicEnergySourceHelper basicSourceHelper;
    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(10.0));
    EnergySourceContainer sources = basicSourceHelper.Install(mobileDevices);

    DeviceEnergyModelHelper deviceEnergyHelper;
    deviceEnergyHelper.Set("CpuIdlePowerW", DoubleValue(0.1));
    for (uint32_t i = 0; i < mobileDevices.GetN(); ++i) {
    deviceEnergyHelper.Install(devices.Get(i), sources.Get(i));
    }

    // Task Decision Logic
    for (uint32_t i = 0; i < N; ++i) {
    Ptr<Node> device = mobileDevices.Get(i);
    double localEnergy = sources.Get(i)->GetRemainingEnergy();
    double remoteLatency = pointToPoint.GetChannel()->GetDelay().GetSeconds();

        for (uint32_t j = 0; j < K; ++j) {
        double dataSize = dataSizeVar->GetValue();
        double cpuPower = cpuPowerVar->GetValue();

        // Simple decision logic: if local energy is enough and latency is high, compute locally
        if (localEnergy > 0.5 && remoteLatency > 0.01) {
            // Execute locally
            NS_LOG_INFO("Device " << i << ", Task " << j << " executes locally.");
        } else {
            // Execute remotely
            NS_LOG_INFO("Device " << i << ", Task " << j << " executes remotely.");
            }
        }
    }

    // 계산 작업 설정
    for (uint32_t i = 0; i < N; ++i)
    {
        for (uint32_t j = 0; j < K; ++j)
        {
            Ptr<UniformRandomVariable> dataSizeVar = CreateObject<UniformRandomVariable>();
            dataSizeVar->SetAttribute("Min", DoubleValue(0));
            dataSizeVar->SetAttribute("Max", DoubleValue(10)); // MB

            Ptr<UniformRandomVariable> cpuPowerVar = CreateObject<UniformRandomVariable>();
            cpuPowerVar->SetAttribute("Min", DoubleValue(0.5));
            cpuPowerVar->SetAttribute("Max", DoubleValue(1.0)); // GHz

            double dataSize = dataSizeVar->GetValue();
            double cpuPower = cpuPowerVar->GetValue();

            NS_LOG_INFO("기기 " << i << ", 작업 " << j << ": 데이터 크기 = " << dataSize << " MB, CPU = " << cpuPower << " GHz");
        }
    }

    

    // 여기에 에너지 모델 설정 추가 가능

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
