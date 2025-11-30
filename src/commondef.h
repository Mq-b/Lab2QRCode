#pragma once 
#include <QString>
#include <QImage>
#include <ZXing/Barcode.h>
class QStandardItemModel;
class QTableView;
class QLabel;
class QStatusBar;
class QMenuBar;
class QMenu;
class QVBoxLayout;
class FrameWidget;
class CameraVideoSurface;
class QCheckBox;

struct FrameResult {
    QString type;
    QString content;
    QVector<QPoint> points; // 存放二维码四个角点，顺序固定 TL, TR, BR, BL
};