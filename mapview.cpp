#include "mapview.h"
#include <QPainter>
#include <QtMath>
#include <QFile>
#include <QDebug>
#include <queue>
#include <limits>
#include <cmath>
#include <QTransform>

constexpr int MapView::TILE_SIZE;

MapView::MapView(QWidget *parent)
    : QWidget(parent),
      m_centerLat(0), m_centerLon(0), m_myLat(0), m_myLon(0), m_zoom(18)
{
    setMinimumSize(400, 300);
    m_myPosImage = QPixmap(":/images/Navi_pos.png");
}


void MapView::setMapData(const QVector<Node>& nodes, const QVector<Way>& ways)
{
    m_nodes = nodes;
    m_ways = ways;
    m_nodeMap.clear();

    for (const Node& n : m_nodes)
        m_nodeMap.insert(n.id, n);
    m_graph = buildGraph(m_nodes, m_ways);  // 여기에 그래프 생성 및 저장


    if (!m_nodes.isEmpty()) {
        m_centerLat = m_nodes.first().lat;
        m_centerLon = m_nodes.first().lon;
        m_myLat = m_centerLat;
        m_myLon = m_centerLon;
    }
    update();
}

void MapView::setCurrentLocation(double lat, double lon)
{
    m_myLat = lat;
    m_myLon = lon;
    m_centerLat = lat;
    m_centerLon = lon;
    update();
}

QPointF MapView::latLonToTileXY(double lat, double lon, int zoom)
{
    double n = qPow(2.0, zoom);
    double x = (lon + 180.0) / 360.0 * n;
    double latRad = qDegreesToRadians(lat);
    double y = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / 3.14159265358979323846) / 2.0 * n;
    return QPointF(x, y);
}

QPointF MapView::latlonToPixel(double lat, double lon) const
{
    double scale = (1 << m_zoom) * TILE_SIZE;
    double siny = qSin(qDegreesToRadians(lat));
    siny = qBound(-0.9999, siny, 0.9999);
    double px = (lon + 180.0) / 360.0 * scale;
    double py = (0.5 - log((1 + siny) / (1 - siny)) / (4 * 3.14159265358979323846)) * scale;
    return QPointF(px, py);
}
#if 1
void MapView::paintEvent(QPaintEvent *)
{
    //qDebug() << "drawing map";
    constexpr double SCALE_FACTOR = 2.0;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);    // 배율추가!!!
    painter.fillRect(rect(), Qt::white);

    int w = width();
    int h = height();
    int halfWidth = w / 2;
    int halfHeight = h / 2;

    // 화면 중앙으로 이동
    painter.translate(halfWidth, halfHeight);

    // 진행 방향 반대 각도로 지도 회전 (내 진행 방향이 화면 위쪽)
    painter.rotate(-m_currentHeadingDegrees);

    // 원점 복귀 (회전 중심 유지)
    painter.translate(-halfWidth, -halfHeight);

    // 타일 크기 확대 추가!!!
    QPointF centerTile = latLonToTileXY(m_centerLat, m_centerLon, 18);  // 배율추가!!
    // 배율추가!!!
    int scaledTileSize = TILE_SIZE * SCALE_FACTOR;
    int tilesOnHalfWidth  = halfWidth  / scaledTileSize + 2;
    int tilesOnHalfHeight = halfHeight / scaledTileSize + 2;


    int minX = int(centerTile.x()) - tilesOnHalfWidth;
    int maxX = int(centerTile.x()) + tilesOnHalfWidth;
    int minY = int(centerTile.y()) - tilesOnHalfHeight;
    int maxY = int(centerTile.y()) + tilesOnHalfHeight;

    //qDebug() << "Drawing tiles in range:" << minX << "~" << maxX << ", " << minY << "~" << maxY;

    for (int tx = minX; tx <= maxX; ++tx) {
        for (int ty = minY; ty <= maxY; ++ty) {
            QString tilePath = QString("/run/media/mmcblk1p1/tiles_new_z18/%1/%2.png").arg(tx).arg(ty);

            QPixmap px;
            bool loaded = QFile::exists(tilePath) && px.load(tilePath);
            if (QFile::exists(tilePath)) loaded = px.load(tilePath);
             QPoint drawPos( (tx - centerTile.x()) * scaledTileSize + halfWidth,
                            (ty - centerTile.y()) * scaledTileSize + halfHeight);

            if (loaded) painter.drawPixmap(drawPos, px.scaled(scaledTileSize, scaledTileSize));     
            else {
                qDebug() << "Tile not found:" << tilePath;

                QPixmap grayTile(scaledTileSize, scaledTileSize);
                grayTile.fill(Qt::lightGray);
                painter.drawPixmap(drawPos, grayTile);
            }           
        }
    }

    // 중심 픽셀 (확대 반영 없이 원래 값 구한 뒤 확대 적용)
    QPointF centerPix = latlonToPixel(m_centerLat, m_centerLon) * SCALE_FACTOR;

    //qDebug() << "Drawing roads count:" << m_ways.size();
    // 도로
    painter.setPen(QPen(Qt::gray, 3));
    for (const Way &way : m_ways) {
        QVector<QPointF> polyline;
        for (qint64 nodeId : way.node_ids) {
            if (!m_nodeMap.contains(nodeId)) continue;
            const Node &node = m_nodeMap[nodeId];

            QPointF pt = latlonToPixel(node.lat, node.lon) * SCALE_FACTOR
                        - centerPix + QPointF(halfWidth, halfHeight);
            polyline.append(pt);
        }
        if (polyline.size() >= 2)
            painter.drawPolyline(polyline.data(), polyline.size());
    }

    // 경로
    #if 0
    if (!m_routeNodeIds.isEmpty()) {
        QVector<QPointF> routePoints;
        for (qint64 nodeId : m_routeNodeIds) {
            if (!m_nodeMap.contains(nodeId)) continue;
            const Node &node = m_nodeMap[nodeId];
            QPointF pt = latlonToPixel(node.lat, node.lon) * SCALE_FACTOR
                        - centerPix + QPointF(halfWidth, halfHeight);
            routePoints.append(pt);
        }
        if (routePoints.size() >= 2) {
            painter.setPen(QPen(Qt::green, 8 * SCALE_FACTOR, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPolyline(routePoints.data(), routePoints.size());
        }
    }
    #endif
    if (!m_routeNodeIds.isEmpty()) {        
        // m_currentRouteIdx가 현재 위치를 나타내는 인덱스라고 가정
        int startIdx = m_currentRouteIdx;
        if (startIdx < 0) startIdx = 0;
        if (startIdx >= m_routeNodeIds.size() - 1) return;

        QVector<QPointF> routePoints;
        for (int i = startIdx; i < m_routeNodeIds.size(); ++i) {
            qint64 nodeId = m_routeNodeIds[i];
            if (!m_nodeMap.contains(nodeId)) continue;
            const Node &node = m_nodeMap[nodeId];
            QPointF pt = latlonToPixel(node.lat, node.lon) * SCALE_FACTOR
                        - centerPix + QPointF(halfWidth, halfHeight);
            routePoints.append(pt);
        }
        if (routePoints.size() >= 2) {
            painter.setPen(QPen(Qt::green, 8 * SCALE_FACTOR, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPolyline(routePoints.data(), routePoints.size());
        }
    }

    // 내 위치
    QPointF myPos(halfWidth, halfHeight);

    // 이미지 크기를 원하는대로 조절 (예: 24x24 픽셀)
    int imgWidth = 48 * SCALE_FACTOR;
    int imgHeight = 48 * SCALE_FACTOR;

    // 이미지 중심이 화면 중심에 위치하도록 위치 조정
    QPoint topLeft(myPos.x() - imgWidth / 2, myPos.y() - imgHeight / 2);

    painter.drawPixmap(topLeft, m_myPosImage.scaled(imgWidth, imgHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

#endif

// 휠 이벤트 무시(줌 레벨 고정)
void MapView::wheelEvent(QWheelEvent *event) {  }

// 그래프 빌드 다익스트라 알고리즘 함수 (static 혹은 별도)
QMap<qint64, QVector<QPair<qint64, double>>> buildGraph(const QVector<Node> &nodes, const QVector<Way> &ways)
{
    QMap<qint64, QVector<QPair<qint64, double>>> graph;
    QHash<qint64, Node> nodeMap;
    for (const Node &n : nodes) nodeMap[n.id] = n;
int totalEdges = 0;

    for (const Way &way : ways) {
        for (int i = 1; i < way.node_ids.size(); ++i) {
            qint64 from = way.node_ids[i - 1];
            qint64 to = way.node_ids[i];
            if (!nodeMap.contains(from) || !nodeMap.contains(to)) continue;

            const Node &n1 = nodeMap[from];
            const Node &n2 = nodeMap[to];
            double dlat = n1.lat - n2.lat;
            double dlon = n1.lon - n2.lon;
            double dist = std::sqrt(dlat*dlat + dlon*dlon);

            graph[from].append(qMakePair(to, dist));
            graph[to].append(qMakePair(from, dist));
            totalEdges += 2; // 양방향

        }
    }
        qDebug() << "그래프 생성 완료 - 총 간선 수 (양방향 포함):" << totalEdges;

    return graph;

}

// 경로 찾기 (다익스트라)
QVector<qint64> MapView::findShortestPath(qint64 startId, qint64 goalId)
{
    QVector<qint64> result;
    if (!m_nodeMap.contains(startId) || !m_nodeMap.contains(goalId)) return result;
int missingNodesCount = 0;
for (const Way &way : m_ways) {
    for (qint64 nodeId : way.node_ids) {
        if (!m_nodeMap.contains(nodeId)) {
            qDebug() << "[MissingNode] 웨이에 존재하지 않는 노드 ID:" << nodeId;
            missingNodesCount++;
        }
    }
}
qDebug() << "[Check] 존재하지 않는 노드 ID 개수:" << missingNodesCount;
    const auto &graph = m_graph;
    
    qDebug() << "[디버깅] 그래프 생성. 노드 수:" << graph.size()
         << ", 임의 노드 이웃 수:" << graph.value(m_nodes[0].id).size();
    int limit = 30; // 일부 노드만 출력
for (auto it = graph.begin(); it != graph.end() && limit > 0; ++it, --limit) {
    qint64 nodeId = it.key();
    const auto& neighbors = it.value();
    //qDebug() << "노드 ID:" << nodeId << "이웃 수:" << neighbors.size();
}
    

    QHash<qint64, double> dist;
    QHash<qint64, qint64> prev;

    for (const Node &n : m_nodes)
        dist[n.id] = std::numeric_limits<double>::infinity();

    dist[startId] = 0.0;

    using PQItem = QPair<double, qint64>;
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> queue;
    queue.push(qMakePair(0.0, startId));

    qDebug() << "[FindShortestPath] 시작:" << startId << "목적지:" << goalId;
    if (!m_nodeMap.contains(startId) || !m_nodeMap.contains(goalId)) {
    qDebug() << "[Error] 출발 또는 목적지 노드가 지도에 존재하지 않음";
}

    while (!queue.empty()) {
        auto current = queue.top();
        queue.pop();

        double curDist = current.first;
        qint64 curNode = current.second;
        //qDebug() << "[FindShortestPath] 방문 노드:" << curNode << ", 거리:" << curDist;

        if (curNode == goalId)
            break;

        if (curDist > dist[curNode])
            continue;
        if (!graph.contains(curNode))
            continue;

        for (const auto &neighbor : graph[curNode]) {
            qint64 nextNode = neighbor.first;
            double edgeCost = neighbor.second;
            double newDist = curDist + edgeCost;

            if (newDist < dist[nextNode]) {
                dist[nextNode] = newDist;
                prev[nextNode] = curNode;
                queue.push(qMakePair(newDist, nextNode));
            }
        }
    }

    if (!prev.contains(goalId) && startId != goalId)
        return result;

    qDebug() << "[FindShortestPath] 최종 경로 길이:" << result.size();
    qDebug() << "[FindShortestPath] 경로:" << result;   

    qint64 cur = goalId;
    while (cur != startId) {
        result.prepend(cur);
        cur = prev.value(cur, startId);
    }
    result.prepend(startId);

    return result;
}

void MapView::setRouteNodeIds(const QVector<qint64> &route)
{
    m_routeNodeIds = route;
    update();
}

const QVector<qint64>& MapView::getRouteNodeIds() const
{
    return m_routeNodeIds;
}

const Node* MapView::getNodeById(qint64 id) const
{
    auto it = m_nodeMap.find(id);
    if (it != m_nodeMap.end())
        return &it.value();
    return nullptr;
}

qint64 MapView::findNearestNodeId(double lat, double lon) const
{
    if (m_nodes.isEmpty())
        return -1;

    qint64 nearestNodeId = -1;
    double minDist = std::numeric_limits<double>::max();

    for (const Node& node : m_nodes) {
        double dLat = node.lat - lat;
        double dLon = node.lon - lon;
        double dist = dLat * dLat + dLon * dLon;  // 제곱 거리로 계산 (루트 생략)

        if (dist < minDist) {
            minDist = dist;
            nearestNodeId = node.id;
        }
    }
    return nearestNodeId;
}

#if 0
// 출발지와 목적지 주변 노드와 이웃 노드 정보 출력 함수
void printNodeNeighbors(const QMap<qint64, QVector<QPair<qint64, double>>>& graph, qint64 nodeId, int depth = 2) {
    qDebug() << "=== 노드와 주변 " << depth << "단계 이웃 정보: 노드ID =" << nodeId << "===";

    QSet<qint64> visited;
    QVector<qint64> currentLevel;
    currentLevel.append(nodeId);
    visited.insert(nodeId);

    for (int level = 0; level < depth; ++level) {
        QVector<qint64> nextLevel;
        qDebug() << "레벨 " << level << "노드 수:" << currentLevel.size();

        for (qint64 nid : currentLevel) {
            qDebug() << "노드 " << nid << "이웃개수:" << graph.value(nid).size();
            for (const auto& neighbor : graph.value(nid)) {
                qint64 neighborId = neighbor.first;
                double distance = neighbor.second;
                qDebug() << "  -> 이웃노드ID:" << neighborId << ", 거리:" << distance;
                if (!visited.contains(neighborId))
                    nextLevel.append(neighborId);
                visited.insert(neighborId);
            }
        }

        currentLevel = nextLevel;
        if (currentLevel.isEmpty())
            break;  // 더 이상 이웃 없음
    }
}
#endif