#include "simulator.h"
#include "random.h"
#include <cmath>
#include <iostream>
using std::cout; using std::endl;

Simulator::Simulator(QObject *parent) :
    QObject(parent)
{
    resize(100, 100);
    diffusionConstant = 0.1;
    dt = 0.001;
}

Simulator::~Simulator()
{

}

void Simulator::resize(unsigned int binsX, unsigned int binsY)
{
    numberOfBinsX = binsX;
    numberOfBinsY = binsY;
    binSizeX = 1.0/binsX;
    binSizeY = 1.0/binsY;
    wantedWalkersAtBoundary = 1000;
    normalizationFactor = 1.0/(wantedWalkersAtBoundary/numberOfBinsY);
    histogram.resize(binsX*binsY);
}

void Simulator::calculateHistogram()
{
    // Reset bins
    for(float &val : histogram) val = 0;

    // Update bins
    for(unsigned int n=0; n<walkers.size(); n++) {
        QVector2D &position = walkers[n];
        unsigned int binX = position[0]*numberOfBinsX;
        unsigned int binY = position[1]*numberOfBinsY;
        histogram[binIndex1D(binX, binY)]++;
    }
}

unsigned int Simulator::binIndex1D(unsigned int binX, unsigned int binY)
{
    return binX + binY*numberOfBinsX;
}

void Simulator::addWalker(float x0, float x1, float y0, float y1)
{
    float x = x0 + Random::nextDouble()*(x1-x0);
    float y = y0 + Random::nextDouble()*(y1-y0);
    walkers.push_back(QVector2D(x,y));
}

void Simulator::processWalkers()
{
    float standardDeviation = sqrt(dt*diffusionConstant);

    for(unsigned int n=0; n<walkers.size(); n++) {
        QVector2D &position = walkers[n];
        float deltaX = Random::nextGaussian(0, standardDeviation);
        float deltaY = Random::nextGaussian(0, standardDeviation);
        // cout << "Walker stepping " << deltaX << ", " << deltaY << endl;

        position[0] += deltaX;
        position[1] += deltaY;
    }
}

void Simulator::applyBoundaryConditions()
{
    const unsigned int binX = 0;
    const int wantedWalkersPerBin = wantedWalkersAtBoundary / numberOfBinsY;
    for(unsigned int binY=0; binY<numberOfBinsY; binY++) {
        unsigned int index = binIndex1D(binX, binY);
        int numberOfWalkersInBin = histogram[index];
        int deltaWalkers = wantedWalkersPerBin - numberOfWalkersInBin;
        while(deltaWalkers-- > 0) {
            addWalker(0, binSizeX, binY*binSizeY, (binY+1)*binSizeY);
        }
    }

    // Remove all walkers outside the domain
    unsigned int removedWalkers = 0;
    for(unsigned int n=0; n<walkers.size(); n++) {
        QVector2D &position = walkers[n];
        if(position[0] > 1.0 || position[0] < 0.0 || position[1] > 1.0 || position[1] < 0.0) {
            std::swap(walkers.back(), walkers[n]);
            walkers.pop_back();
            n--;
            removedWalkers++;
        }
    }

    calculateHistogram();
}

void Simulator::step()
{
    processWalkers();
    applyBoundaryConditions();
    emit stepCompleted();
}
