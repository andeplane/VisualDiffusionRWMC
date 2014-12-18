#include "scalarfield.h"
#include <QColor>
#include <QRgb>
#include <iostream>

using std::cout; using std::endl;
using std::vector;

ScalarField::ScalarField() :
    m_numPointsX(0),
    m_numPointsY(0),
    m_indicesDirty(true),
    m_funcs(0),
    m_program(0)
{
    generateVBOs();
}

ScalarField::~ScalarField()
{
    if(m_funcs) delete m_funcs;
    if(m_program) delete m_program;
}

void ScalarField::generateVBOs()
{
    ensureInitialized();
    m_funcs->glGenBuffers(2, m_vboIds);
}

void ScalarField::ensureInitialized()
{
    if(!m_funcs) m_funcs = new QOpenGLFunctions(QOpenGLContext::currentContext());
}

void scalarToRGBRainBow(float scalar, float &red, float &green, float &blue)
{
    float scaled = (1.0 - scalar)*4;
    int group = floor(scaled);
    float rest = scaled - group;
    switch(group) {
        case 0: red = 1.0; green = rest; blue = 0; break;
        case 1: red = 1.0-rest; green = 1.0; blue = 0; break;
        case 2: red = 0.0; green = 1.0; blue = rest; break;
        case 3: red = 0.0; green = 1.0-rest; blue = 1.0; break;
        case 4: red = 0.0; green = 0.0; blue = 1.0; break;
    }
}

void ScalarField::update(ScalarFieldData &data)
{
    ensureInitialized();
    if(data.numPointsX == 0) return;
    resize(data.numPointsX, data.numPointsY);
    vector<float> &values = data.values;
    for(unsigned int i=0; i<m_vertices.size(); i++) {
        // Choose color
        float &red = m_vertices[i].color[0];
        float &green = m_vertices[i].color[1];
        float &blue = m_vertices[i].color[2];
        // cout << "Value: " << values[i] << endl;
        scalarToRGBRainBow(values[i], red, green, blue);
    }

    // Transfer vertex data to VBO 0
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(ScalarFieldVertex), &m_vertices[0], GL_STATIC_DRAW);
    if(m_indicesDirty) {
        m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);
        m_funcs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLuint), &m_indices[0], GL_STATIC_DRAW);
        m_indicesDirty = false;
    }
}

void ScalarField::createShaderProgram() {
    if (!m_program) {
        m_program = new QOpenGLShaderProgram();

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute highp vec4 a_position;\n"
                                           "attribute highp vec4 a_color;\n"
                                           "varying highp vec4 color;\n"
                                           "void main() {\n"
                                           "    gl_Position = a_position;\n"
                                           "    color = a_color;\n"
                                           "}");

        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "varying highp vec4 color;\n"
                                           "void main() {\n"
                                           "    gl_FragColor = vec4(color.xyz, 1.0);\n"
                                           "}");


        m_program->link();
    }
}

void ScalarField::render()
{
    if(m_vertices.size() == 0) return;
    ensureInitialized();
    createShaderProgram();

    m_program->bind();

    // Tell OpenGL which VBOs to use
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = m_program->attributeLocation("a_position");
    m_program->enableAttributeArray(vertexLocation);
    m_funcs->glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(ScalarFieldVertex), (const void *)offset);

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex color data
    int colorLocation = m_program->attributeLocation("a_color");
    m_program->enableAttributeArray(colorLocation);
    m_funcs->glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(ScalarFieldVertex), (const void *)offset);

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Draw triangles using indices from VBO 1
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_funcs->glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    // glDisable(GL_BLEND);
    m_program->disableAttributeArray(vertexLocation);
    m_program->disableAttributeArray(colorLocation);

    m_program->release();
}

void ScalarField::resize(unsigned int numPointsX, unsigned int numPointsY)
{
    if(m_numPointsX == numPointsX && m_numPointsY == numPointsY) return; // Not changed
    m_numPointsX = numPointsX;
    m_numPointsY = numPointsY;
    unsigned int numPoints = numPointsX*numPointsY;
    int numTriangles = 2*(numPointsX-1)*(numPointsY-1);
    int numIndices = 3*numTriangles;
    m_vertices.resize(numPoints);
    m_indices.clear();
    m_indices.reserve(numIndices);
    for(int i=0; i<int(numPointsX); i++) {
        for(int j=0; j<int(numPointsY); j++) {
            float x = 2*(float(i)/(numPointsX-1) - 0.5);
            float y = 2*(float(j)/(numPointsY-1) - 0.5);
            float z = 0.0;

            unsigned int idx = index(i, j);
            m_vertices[idx].position = QVector3D(x,y,z);

            if(i < int(numPointsX)-1 && j < int(numPointsY)-1) {
                unsigned int index1 = index(i+1,j);
                unsigned int index2 = index(i,j+1);
                unsigned int index3 = index(i,j);
                m_indices.push_back(index1);
                m_indices.push_back(index2);
                m_indices.push_back(index3);

                index1 = index(i+1,j+1);
                index2 = index(i,j+1);
                index3 = index(i+1,j);
                m_indices.push_back(index1);
                m_indices.push_back(index2);
                m_indices.push_back(index3);
            }
        }
    }
    m_indicesDirty = true;
}
