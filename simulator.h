#ifndef SIMULATOR_H
#define SIMULATOR_H
#include <QObject>
#include <QVector2D>
#include <vector>

class Simulator : public QObject
{
    Q_OBJECT
public:
    unsigned int numberOfBinsX;
    unsigned int numberOfBinsY;
    unsigned int wantedWalkersAtBoundary;
    float        scalingFactor;
    float        binSizeX;
    float        binSizeY;
    float        diffusionConstant;
    float        dt;
    float        normalizationFactor;

    Simulator(QObject* parent = 0);
    ~Simulator();
    std::vector<QVector2D> walkers;
    std::vector<float> histogram;
    std::vector<std::vector<unsigned int> > cellList;
    void resize(unsigned int binsX, unsigned int binsY);
    void calculateHistogram();
    unsigned int binIndex1D(unsigned int binX, unsigned int binY);
    void addWalker(float x0, float x1, float y0, float y1);
    void processWalkers();
    void applyBoundaryConditions();
public slots:
    void step();

signals:
    void stepCompleted();
};

#endif // SIMULATOR_H
