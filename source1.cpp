#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MobileIoTNetworkSimulation");

int main(int argc, char* argv[])
{
    uint32_t N = 100; // ����� ��� ��
    uint32_t K = 4;   // ��� �� �۾� ��
    double serverCpu = 100.0; // GHz, ���� CPU

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // ��� ����
    NodeContainer mobileDevices;
    mobileDevices.Create(N);
    NodeContainer server;
    server.Create(1);

    // ���ͳ� ����
    InternetStackHelper stack;
    stack.Install(mobileDevices);
    stack.Install(server);

    // IP �ּ� �Ҵ�
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(mobileDevices);
    Ipv4InterfaceContainer serverInterface = address.Assign(server);

    // �̵��� ����
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

    // ��� �۾� ����
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

            NS_LOG_INFO("��� " << i << ", �۾� " << j << ": ������ ũ�� = " << dataSize << " MB, CPU = " << cpuPower << " GHz");
        }
    }

    // ���⿡ ������ �� ���� �߰� ����

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}