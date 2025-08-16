#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QWidget>
#include <QVector>
#include <QHash>
#include <QPair>
#include <QPixmap>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QGraphicsView>

struct Node {
    qint64 id;
    double lat, lon;
};

struct Way {
    QVector<qint64> node_ids;
    QString name;
    QString highway_type;
    QString tunnel;
    int maxspeed = -1;
};

QMap<qint64, QVector<QPair<qint64, double>>> buildGraph(const QVector<Node> &nodes, const QVector<Way> &ways);

class MapView : public QWidget
{
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);

    // 지도를 위한 데이터 설정
    void setMapData(const QVector<Node>& nodes, const QVector<Way>& ways);
    void setCurrentLocation(double lat, double lon);

    QVector<qint64> findShortestPath(qint64 startId, qint64 goalId);  // 다익스트라 경로탐색
    void setRouteNodeIds(const QVector<qint64>& route);
    const QVector<qint64>& getRouteNodeIds() const;
    const Node* getNodeById(qint64 id) const;

    qint64 findNearestNodeId(double lat, double lon) const;

    const QMap<qint64, QVector<QPair<qint64, double>>>& getGraph() const { return m_graph; }
    const QVector<Node>& getNodes() const { return m_nodes; }

    //void applyRotation(double headingDegrees);

    //void setMapRotationAngle(double angle);
    
    void setCurrentHeading(double degrees) { 
        m_currentHeadingDegrees = degrees;
        update();  // 변경 시 화면 갱신
    }

    void setCurrentRouteIndex(int idx)
{
    m_currentRouteIdx = idx;
    update();
}


protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override; // 줌 기능 제거

    //void drawBackground(QPainter* painter, const QRectF& rect) override;


private:
    QGraphicsScene* m_scene;  // 씬 포인터 멤버 변수 선언


    QVector<Node> m_nodes;
    QVector<Way> m_ways;
    QHash<qint64, Node> m_nodeMap;
    QMap<qint64, QVector<QPair<qint64, double>>> m_graph;

    QVector<qint64> m_routeNodeIds;

    double m_centerLat;
    double m_centerLon;
    double m_myLat;
    double m_myLon;

    const int m_zoom = 18; // 줌 고정
    static constexpr int TILE_SIZE = 256;

    // 좌표 변환 함수
    static QPointF latLonToTileXY(double lat, double lon, int zoom);
    QPointF latlonToPixel(double lat, double lon) const;

    QHash<QString, QPixmap> m_tileCache;  // 타일 캐시

    double m_mapRotationAngle = 0.0;

    double m_currentHeadingDegrees = 0.0; // 0~360도 진행 방향 각도
    QPixmap m_myPosImage;
    int m_currentRouteIdx = 0;  // 현재 경로 인덱스 (기본값 0)

};

#endif // MAPVIEW_H
