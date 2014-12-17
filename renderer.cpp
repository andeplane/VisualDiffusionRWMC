#include "renderer.h"
#include "controller.h"
#include "points.h"

#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QGLFormat>
#include <QOpenGLContext>
#include <iostream>
#include <cmath>
#include <QOpenGLFramebufferObjectFormat>

using namespace std;
namespace CompPhys {

void Renderer::synchronize(QQuickFramebufferObject* item)
{
    m_syncCount++;
    Controller *controller = (Controller*)item; // TODO: Use correct casting method
    if(!controller) {
        return;
    }
    resetProjection();

    if(controller->simulatorOutputDirty()) {
        controller->m_simulatorOutputMutex.lock();
        // Read output
        m_renderPoints = controller->renderParticles();
        m_renderScalarField = controller->renderScalarField();
        m_data.pointsData = controller->m_visualData.pointsData;
        m_data.scalarFieldData = controller->m_visualData.scalarFieldData;

        m_points->update(m_data.pointsData);
        m_scalarField.update(m_data.scalarFieldData);

        controller->setSimulatorOutputDirty(false);
        controller->m_simulatorOutputMutex.unlock();
        m_dirtyCount++;
    }
}

void Renderer::render()
{
    m_renderCount++;
    glDepthMask(true);

    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    // Render data
    if(m_renderScalarField) m_scalarField.render();
    if(m_renderPoints) m_points->render();

    // glDepthMask(GL_TRUE);

    glDisable(GL_CULL_FACE);
}


Renderer::Renderer() :
    m_skipNextFrame(false),
    m_syncCount(0),
    m_renderCount(0),
    m_dirtyCount(0),
    m_renderPoints(true),
    m_renderScalarField(true)
{
    m_points = new Points();
}

Renderer::~Renderer()
{

}

void Renderer::resetProjection()
{
    // Calculate aspect ratio
    qreal aspect = qreal(m_viewportSize.width()) / qreal(m_viewportSize.height() ? m_viewportSize.height() : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 65 degrees
    const qreal zNear = 2.0, zFar = 2000.0, fov = 65.0;

    // Reset projection
    m_projectionMatrix.setToIdentity();

    // Set perspective projection
    m_projectionMatrix.perspective(fov, aspect, zNear, zFar);
}

void Renderer::setModelViewMatrices(double zoom, double tilt, double pan, double roll, const QVector3D &systemSize)
{
    m_systemSize = systemSize;
    float systemSizeX = systemSize.x();
    float systemSizeY = systemSize.y();
    float systemSizeZ = systemSize.z();
    float systemSizeMax = sqrt(systemSizeX*systemSizeX + systemSizeY*systemSizeY + systemSizeZ*systemSizeZ);

    m_modelViewMatrix.setToIdentity();
    m_modelViewMatrix.translate(0,0,zoom);
    m_modelViewMatrix.rotate(90, 1, 0, 0);
    m_modelViewMatrix.rotate(tilt, 1, 0, 0);
    m_modelViewMatrix.rotate(pan, 0, 0, 1);
    m_modelViewMatrix.rotate(roll, 0, 1, 0);

    m_lightModelViewMatrix.setToIdentity();
    m_lightModelViewMatrix.translate(0,0,-systemSizeMax / 2.0);
    m_lightModelViewMatrix.rotate(90, 1, 0, 0);
    m_lightModelViewMatrix.rotate(tilt, 1, 0, 0);
    m_lightModelViewMatrix.rotate(pan, 0, 0, 1);
    m_lightModelViewMatrix.rotate(roll, 0, 1, 0);
}

QOpenGLFramebufferObject *Renderer::createFramebufferObject(const QSize &size) {
    m_viewportSize = size;
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}
}
