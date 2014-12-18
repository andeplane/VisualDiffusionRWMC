/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "controller.h"
#include "renderer.h"

#include <functional>
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
Controller::Controller()
    :
      m_running(true),
      m_stepRequested(false),
      m_previousStepCompleted(true),
      m_simulatorOutputDirty(false),
      m_lastStepWasBlocked(false),
      m_renderParticles(true),
      m_renderScalarField(true)
{
    m_simulator.moveToThread(&m_simulatorWorker);
    connect(this, &Controller::requestStep, &m_simulator, &Simulator::step);
    connect(&m_simulator, &Simulator::stepCompleted, this, &Controller::finalizeStep);
    m_simulatorWorker.start();
    m_timer.start();
}

Controller::~Controller()
{
    // m_simulatorInputMutex.unlock();
    // m_simulatorOutputMutex.unlock();
    // m_simulatorRunningMutex.unlock();
    m_simulatorWorker.quit();
    m_simulatorWorker.wait();
}

CompPhys::Renderer* Controller::createRenderer() const
{
    return new CompPhys::Renderer();
}

void Controller::setRunning(bool arg)
{
    if (m_running == arg)
        return;

    m_running = arg;
    emit runningChanged(arg);
    update();
}

void Controller::step()
{
    if(!m_running) {
        return;
    }

    if(m_simulatorRunningMutex.tryLock()) {
        m_lastStepWasBlocked = false;
        emit requestStep();
    } else {
        m_lastStepWasBlocked = true;
    }
}

void Controller::scalePosition(QVector2D &position) {
    // Will scale positions in the range [0, 1] to [-1, 1]
    position[0] = 2*(position[0] - 0.5);
    position[1] = 2*(position[1] - 0.5);
}

void Controller::updateWalkerPositions() {
    // Copy all positions from simulator and scale them
    m_visualData.pointsData.positions = m_simulator.walkers;
    for(QVector2D &position : m_visualData.pointsData.positions) {
        scalePosition(position);
    }
}

void Controller::updateHistogram() {
    // Copy all positions from simulator and scale them
    m_visualData.scalarFieldData.numPointsX = m_simulator.numberOfBinsX;
    m_visualData.scalarFieldData.numPointsY = m_simulator.numberOfBinsY;
    m_visualData.scalarFieldData.values.resize(m_simulator.histogram.size());

    std::function<unsigned int(int, int)> indexPeriodic = [&](int i, int j) {
        return ((i + m_simulator.numberOfBinsX) % m_simulator.numberOfBinsX) + ((j + m_simulator.numberOfBinsY) % m_simulator.numberOfBinsY)*m_simulator.numberOfBinsX;
    };

    for(int i=0; i<int(m_simulator.numberOfBinsX); i++) {
        for(int j=0; j<int(m_simulator.numberOfBinsY); j++) {
            unsigned int index0 = i + j*m_simulator.numberOfBinsX;
            unsigned int index1 = indexPeriodic(i-1, j);
            unsigned int index2 = indexPeriodic(i-1, j-1);
            unsigned int index3 = indexPeriodic(i, j-1);
            float newValue = 0.25*(m_simulator.histogram[index0] + m_simulator.histogram[index1] + m_simulator.histogram[index2] + m_simulator.histogram[index3])*m_simulator.normalizationFactor;
            m_visualData.scalarFieldData.values[index0] *= 0.95;
            m_visualData.scalarFieldData.values[index0] += 0.05*newValue;
        }
    }
}

void Controller::finalizeStep()
{

    m_simulatorOutputMutex.lock();

    // Write output from simulator
    updateWalkerPositions();
    updateHistogram();

    m_previousStepCompleted = true;
    m_simulatorOutputDirty = true;
    m_simulatorOutputMutex.unlock();
    m_simulatorRunningMutex.unlock();
    update();

    if(m_lastStepWasBlocked) {
        m_lastStepWasBlocked = false;
        step();
    }
}

void Controller::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        //        connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
        //        connect(win, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
        // If we allow QML to do the clearing, they would clear what we paint
        // and nothing would show.
        win->setClearBeforeRendering(false);
    }
}

// ********************************************
// ******* Basic setters and getters **********
// ********************************************

void Controller::setPreviousStepCompleted(bool arg)
{
    if (m_previousStepCompleted != arg) {
        m_previousStepCompleted = arg;
        emit previousStepCompletedChanged(arg);
    }
}

void Controller::setSimulatorOutputDirty(bool arg)
{
    m_simulatorOutputDirty = arg;
}

bool Controller::running() const
{
    return m_running;
}

bool Controller::previousStepCompleted() const
{
    return m_previousStepCompleted;
}

bool Controller::simulatorOutputDirty() const
{
    return m_simulatorOutputDirty;
}
}
