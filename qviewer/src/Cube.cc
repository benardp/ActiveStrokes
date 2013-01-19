/*****************************************************************************\

Cube.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
Copyright (c) 2012 Pierre Benard

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Cube.h"

// Positions
GLfloat CubeArray[72] =  {1,1,1,  -1,1,1,  -1,-1,1,  1,-1,1,        // v0-v1-v2-v3
                          1,1,1,  1,-1,1,  1,-1,-1,  1,1,-1,        // v0-v3-v4-v5
                          1,1,1,  1,1,-1,  -1,1,-1,  -1,1,1,        // v0-v5-v6-v1
                          -1,1,1,  -1,1,-1,  -1,-1,-1,  -1,-1,1,    // v1-v6-v7-v2
                          -1,-1,-1,  1,-1,-1,  1,-1,1,  -1,-1,1,    // v7-v4-v3-v2
                          1,-1,-1,  -1,-1,-1,  -1,1,-1,  1,1,-1};   // v4-v7-v6-v5

// Colors
GLfloat CubeColor[72] =  {1,1,1,  1,1,0,  1,0,0,  1,0,1,              // v0-v1-v2-v3
                          1,1,1,  1,0,1,  0,0,1,  0,1,1,              // v0-v3-v4-v5
                          1,1,1,  0,1,1,  0,1,0,  1,1,0,              // v0-v5-v6-v1
                          1,1,0,  0,1,0,  0,0,0,  1,0,0,              // v1-v6-v7-v2
                          0,0,0,  0,0,1,  1,0,1,  1,0,0,              // v7-v4-v3-v2
                          0,0,1,  0,0,0,  0,1,0,  0,1,1};             // v4-v7-v6-v5

// Normals
GLfloat CubeNormal[72] = {0,0,1,  0,0,1,  0,0,1,  0,0,1,             // v0-v1-v2-v3
                          1,0,0,  1,0,0,  1,0,0, 1,0,0,              // v0-v3-v4-v5
                          0,1,0,  0,1,0,  0,1,0, 0,1,0,              // v0-v5-v6-v1
                          -1,0,0,  -1,0,0, -1,0,0,  -1,0,0,          // v1-v6-v7-v2
                          0,-1,0,  0,-1,0,  0,-1,0,  0,-1,0,         // v7-v4-v3-v2
                          0,0,-1,  0,0,-1,  0,0,-1,  0,0,-1};        // v4-v7-v6-v5

// Indices
GLuint IndicesArray[24] = {0,1,2,3,
                          4,5,6,7,
                          8,9,10,11,
                          12,13,14,15,
                          16,17,18,19,
                          20,21,22,23};

Cube::Cube()
{
    _radius = 2.4;

    glGenBuffers(5, (GLuint *)_cubeBuffers);

    glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CubeArray), CubeArray, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CubeColor), CubeColor, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffers[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CubeNormal), CubeNormal, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _cubeBuffers[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndicesArray), IndicesArray, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    _initVAO = false;

    reportGLError();
}

void Cube::createVAO(GQShaderRef& shader)
{
    _vertexPositionIndex = shader.attribLocation("vertex");
    _vertexColorIndex    = shader.attribLocation("color");
    _vertexNormalIndex   = shader.attribLocation("normal");

    if(!_initVAO){
        glGenVertexArrays(1, (GLuint *)&_vertexArrayObject);
    }

    glBindVertexArray(_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffers[0]);
    glVertexAttribPointer(_vertexPositionIndex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(_vertexPositionIndex);

    reportGLError();

    if(_vertexColorIndex >=0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffers[1]);
        glVertexAttribPointer(_vertexColorIndex, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_vertexColorIndex);
        reportGLError();
    }
    if(_vertexNormalIndex >=0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffers[2]);
        glVertexAttribPointer(_vertexNormalIndex, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_vertexNormalIndex);
        reportGLError();
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _cubeBuffers[3]);

    glBindVertexArray(0);

    reportGLError();

    _initVAO = true;
}

void Cube::draw(GQShaderRef& shader)
{
    createVAO(shader);

    glBindVertexArray(_vertexArrayObject);
    glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    reportGLError();
}
