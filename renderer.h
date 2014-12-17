#pragma once

#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QThread>
#include <QMutex>
#include "points.h"
#include "scalarfield.h"

namespace CompPhys {
struct VisualData {
    PointsData pointsData;
    ScalarFieldData scalarFieldData;
};

class Renderer : public QQuickFramebufferObject::Renderer
{
public:
    Renderer();
    ~Renderer();

    void setViewportSize(const QSize &size) { m_viewportSize = size; }
    void resetProjection();
    void setModelViewMatrices(double zoom, double tilt, double pan, double roll, const QVector3D &systemSize);

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);
    void synchronize(QQuickFramebufferObject *item);
    void render();

private:
    QSize m_viewportSize;

    QVector3D  m_systemSize;
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_modelViewMatrix;
    QMatrix4x4 m_lightModelViewMatrix;

    bool m_skipNextFrame;
    bool m_renderPoints;
    bool m_renderScalarField;
    Points *m_points;
    ScalarField m_scalarField;
    VisualData m_data;

    int m_syncCount;
    int m_renderCount;
    int m_dirtyCount;
};
}
