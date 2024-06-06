#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <fstream>
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LteMecSimulation");

void RunSimulation(int scenario);

int main (int argc, char *argv[])
{
  // 로그 컴포넌트 초기화
  LogComponentEnable ("LteMecSimulation", LOG_LEVEL_INFO);

  // 각 시나리오 실행
  for (int scenario = 1; scenario <= 4; scenario++) {
    RunSimulation(scenario);
  }

  return 0;
}

void RunSimulation(int scenario) {
  // LTE 통신 네트워크 설정
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // RemoteHost 노드 생성 및 인터넷 연결 설정
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  // RemoteHost 노드에 라우팅 설정
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // eNodeB와 Ue 노드 생성
  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (1);
  ueNodes.Create (100);

  // 모바일 장치와 eNodeB 위치 설정
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  mobility.Install (ueNodes);

  // LTE 장치 설치
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // IP 주소 할당
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // 모든 Ue 장치를 eNodeB에 연결
  lteHelper->Attach (ueLteDevs, enbLteDevs.Get (0));

  // MEC 서버 설정 (여기서는 RemoteHost를 MEC 서버로 가정)
  uint16_t mecPort = 1234;

  // MEC 서버 애플리케이션 설치 (RemoteHost에 설치)
  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), mecPort));
  ApplicationContainer serverApps = packetSinkHelper.Install (remoteHost);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // 각 모바일 장치의 작업 설정
  std::default_random_engine generator;
  std::uniform_real_distribution<double> dataSizeDistribution(0.0, 10.0);
  std::uniform_int_distribution<int> cpuIndexDistribution(5, 10); // 5부터 10까지 정수 생성
  std::uniform_real_distribution<double> energyDistribution(0.0, 20e-11);

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      double cpuCapability = cpuIndexDistribution(generator) * 0.1; // 0.5 ~ 1.0 범위에서 0.1 단위로 랜덤 값
      for (uint32_t task = 0; task < 4; ++task)
        {
          double dataSize = dataSizeDistribution(generator) * 1e6; // MB to Bytes
          double localEnergyPerCycle = energyDistribution(generator);
          int appType = task % 6; // 간단히 하기 위해 6개의 앱 타입 순환

          NS_LOG_INFO("Scenario " << scenario << " - UE " << u << " Task " << task << " DataSize: " << dataSize << " Bytes CPU Capability: " << cpuCapability << " GHz Local Energy Per Cycle: " << localEnergyPerCycle << " J/Cycle App Type: " << appType);

          uint32_t packetSize = 1400; // bytes
          uint32_t numPackets = dataSize / packetSize;

          // 시나리오에 따라 오프로드 결정
          bool offload = false;
          switch (scenario) {
            case 1: // Local Execution (LE)
              offload = false;
              break;
            case 2: // Full Offloading (FO)
              offload = true;
              break;
            case 3: // Computation Offloading (CO)
              // 최적화 모델에 따라 결정, 여기서는 예시로 랜덤하게 결정
              offload = (rand() % 2 == 0);
              break;
            case 4: // Proposed Model
              // 제안된 모델에 따라 결정, 여기서는 예시로 랜덤하게 결정
              offload = (rand() % 2 == 0);
              break;
          }

          if (offload) {
            UdpClientHelper client (remoteHostAddr, mecPort);
            client.SetAttribute ("Interval", TimeValue (MilliSeconds (100)));
            client.SetAttribute ("MaxPackets", UintegerValue (numPackets));
            client.SetAttribute ("PacketSize", UintegerValue (packetSize));

            ApplicationContainer clientApps = client.Install (ueNodes.Get (u));
            clientApps.Start (Seconds (1.0));
            clientApps.Stop (Seconds (10.0));
          } else {
            // 로컬 실행 로직 추가
            // 여기서는 예시로 패킷을 전송하지 않고 로컬에서 처리한다고 가정
            NS_LOG_INFO("Task " << task << " on UE " << u << " executed locally.");
          }
        }
    }

  // 플로우 모니터 설정
  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> flowMonitor = flowmonHelper.InstallAll ();

  // 시뮬레이션 실행
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();

  // 플로우 모니터 결과 출력
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();

  std::ofstream out ("flow_monitor_results_scenario_" + std::to_string(scenario) + ".txt");
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      out << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
      out << "  Tx Bytes:   " << i->second.txBytes << "\n";
      out << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      out << "  Tx Packets: " << i->second.txPackets << "\n";
      out << "  Rx Packets: " << i->second.rxPackets << "\n";
      out << "  Lost Packets: " << i->second.lostPackets << "\n";
      out << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024 << " Mbps\n";
    }
  out.close ();

  // 시뮬레이터 종료
  Simulator::Destroy ();
}







