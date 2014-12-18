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

    if(controller->simulatorOutputDirty()) {
        controller->m_simulatorOutputMutex.lock();
        // Read output
        m_renderPoints = controller->renderParticles();
        m_renderScalarField = controller->renderScalarField();
        m_data.pointsData = controller->m_visualData.pointsData;
        m_data.scalarFieldData = controller->m_visualData.scalarFieldData;

        m_points.update(m_data.pointsData);
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

    glDisable(GL_DEPTH_TEST);
    // Render data
    if(m_renderScalarField) m_scalarField.render();
    if(m_renderPoints) m_points.render();
}


Renderer::Renderer() :
    m_skipNextFrame(false),
    m_renderPoints(true),
    m_renderScalarField(true),
    m_syncCount(0),
    m_renderCount(0),
    m_dirtyCount(0)
{

}

Renderer::~Renderer()
{
    m_data.pointsData.positions.clear();
    m_data.scalarFieldData.values.clear();
}

QOpenGLFramebufferObject *Renderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}
}
