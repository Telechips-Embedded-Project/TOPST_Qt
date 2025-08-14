#include "navi_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "navi_utils.h"   // calculateBearing 선언 포함

#include <QDebug>
#include <cmath>
#include <limits>

void printNodeNeighbors(const QMap<qint64, QVector<QPair<qint64, double>>>& graph, qint64 nodeId, int depth = 2) {
    QSet<qint64> visited;
    QVector<qint64> currentLevel{nodeId};
    visited.insert(nodeId);

    for (int level = 0; level < depth && !currentLevel.isEmpty(); ++level) {
        QVector<qint64> nextLevel;
        qDebug() << "레벨" << level << "노드수:" << currentLevel.size();

        for (qint64 nid : currentLevel) {
            qDebug() << "노드" << nid << "이웃수:" << graph.value(nid).size();
            for (const auto &neighbor : graph.value(nid)) {
                qDebug() << " -> 이웃 노드" << neighbor.first << "거리" << neighbor.second;
                if (!visited.contains(neighbor.first))
                    nextLevel.append(neighbor.first);
                visited.insert(neighbor.first);
            }
        }
        currentLevel = nextLevel;
    }
}

bool isConnected(const QMap<qint64, QVector<QPair<qint64, double>>>& graph, qint64 startNode, qint64 goalNode) {
    if (!graph.contains(startNode) || !graph.contains(goalNode)) return false;
    QSet<qint64> visited;
    QVector<qint64> stack{startNode};

    while (!stack.isEmpty()) {
        qint64 node = stack.back();
        stack.pop_back();
        if (node == goalNode) return true;
        if (visited.contains(node)) continue;
        visited.insert(node);
        for (const auto &neighbor : graph[node]) {
            if (!visited.contains(neighbor.first))
                stack.push_back(neighbor.first);
        }
    }
    return false;
}

constexpr double EARTH_RADIUS_M = 6371000.0;

double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    auto deg2rad = [](double deg) { return deg * M_PI / 180.0; };

    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);

    double rLat1 = deg2rad(lat1);
    double rLat2 = deg2rad(lat2);

    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(rLat1) * std::cos(rLat2) * std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    return EARTH_RADIUS_M * c; // 미터 단위 거리
}

qint64 findNearestReachableNode(double lat, double lon,
                               const QVector<Node>& nodes,
                               const QMap<qint64, QVector<QPair<qint64, double>>>& graph) 
{
    qint64 bestNodeId = -1;
    double bestDist = std::numeric_limits<double>::max();

    for (const Node& n : nodes) {
        if (!graph.contains(n.id)) 
            continue;  // 그래프에 포함되지 않은 노드 스킵
        
        if (graph.value(n.id).isEmpty())
            continue;  // 연결된 이웃이 없는 고립 노드 스킵

        double dist = haversineDistance(lat, lon, n.lat, n.lon);
        if (dist < bestDist) {
            bestDist = dist;
            bestNodeId = n.id;
        }
    }
    return bestNodeId;
}


void initNaviConnections(MainWindow* w)
{
    QWidget* pageNavi = w->getUi()->EnterWidget->widget(3);
    QListWidget* destList = pageNavi ? pageNavi->findChild<QListWidget*>("list_dest_places") : nullptr;

    // ----- home, office 버튼 추가 및 초기화 -----
    QPushButton* btnHome = pageNavi->findChild<QPushButton*>("dest_home");
    QPushButton* btnOffice = pageNavi->findChild<QPushButton*>("dest_office");

    if (btnHome) {
        btnHome->setIcon(QIcon(":/images/Navi_home.png")); // 리소스 또는 파일 경로
        btnHome->setIconSize(QSize(128, 128));
        /*
        QObject::connect(btnHome, &QPushButton::clicked, [this]() {
            double homeLat = 37.4777;  // 집 좌표 예시
            double homeLon = 126.8805;
            startNavigationTo(homeLat, homeLon);
        });
        */
    }

    if (btnOffice) {
        btnOffice->setIcon(QIcon(":/images/Navi_office.png"));
        btnOffice->setIconSize(QSize(128, 128));
        /*
        QObject::connect(btnOffice, &QPushButton::clicked, [this]() {
            double officeLat = 37.554722; // 회사 좌표 예시
            double officeLon = 126.970833;
            startNavigationTo(officeLat, officeLon);
        });
        */
    }


    if (!destList || !w->getMapView())
        return;

    // 출발지 고정 좌표 (가산)
    double startLat = 37.4777;
    double startLon = 126.8805;

    //double startLat = 37.4067134067601;
    //double startLon = 127.087301860142;

    // 목적지 좌표 매핑
    QMap<QString, QPair<double,double>> placeCoords = {
        {"list1",  {37.5133, 127.1078}},
        {"list2",  {37.554722, 126.970833}},
        {"list3",  {37.497942, 127.027621}},
        {"gadi",   {37.4810, 126.8820}},
        {"pangyo", {37.3860, 127.1110}}
        // 필요한 목적지를 추가할 수 있습니다.
    };

    QObject::connect(destList, &QListWidget::itemClicked, [w, startLat, startLon, placeCoords](QListWidgetItem* item){
        if (!item) return;

        QString placeName = item->text();
        if (!placeCoords.contains(placeName)) {
            qDebug() << "지원하지 않는 목적지:" << placeName;
            return;
        }
        double goalLat = placeCoords[placeName].first;

        double goalLon = placeCoords[placeName].second;

        double bearing = calculateBearing(startLat, startLon, goalLat, goalLon);
        qDebug() << "출발지에서 목적지 방향(방위각):" << bearing << "도";

        MapView* mapView = w->getMapView();

        const auto& nodes = mapView->getNodes();
        const auto& graph = mapView->getGraph();

        qint64 startNodeId = findNearestReachableNode(startLat, startLon, nodes, graph);
        qDebug() << "[노드 매핑] 출발 좌표:" << startLat << startLon << "→ 노드ID:" << startNodeId;

        qint64 goalNodeId = findNearestReachableNode(goalLat, goalLon, nodes, graph);
        qDebug() << "[노드 매핑] 목적 좌표:" << goalLat << goalLon << "→ 노드ID:" << goalNodeId;

        if (startNodeId < 0 || goalNodeId < 0) {
            qDebug() << "길 찾기 가능한 노드를 찾지 못했습니다.";
            return;
        }

        QVector<qint64> route = mapView->findShortestPath(startNodeId, goalNodeId);

        if (route.isEmpty()) {
            qDebug() << "경로를 찾지 못함";
            return;
        }
       
        mapView->setRouteNodeIds(route);
        w->startRouteSimulation();
    });
}

#if 0
void handleNaviListSelection(MainWindow* w, QListWidgetItem* item)
{
    if (!item) return;
    qDebug() << "Selected navigation place:" << item->text();

    // 선택된 장소에 따라 지도 위치 이동 같은 동작 수행
    // 예:
    if (item->text() == "우리집") {
        w->getMapView->setCurrentLocation(37.5665, 126.9780);
    } else if (item->text() == "사무실") {
        w->getMapView->setCurrentLocation(37.1234, 127.5678);
    }
    // 추가 장소 처리...
}
#endif

