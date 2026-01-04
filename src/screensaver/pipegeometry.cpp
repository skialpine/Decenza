#include "pipegeometry.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// PipeCylinderGeometry - Cylinder with configurable sides
// ============================================================================

PipeCylinderGeometry::PipeCylinderGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    updateGeometry();
}

void PipeCylinderGeometry::setRadius(float radius)
{
    if (qFuzzyCompare(m_radius, radius))
        return;
    m_radius = radius;
    updateGeometry();
    emit radiusChanged();
}

void PipeCylinderGeometry::setLength(float length)
{
    if (qFuzzyCompare(m_length, length))
        return;
    m_length = length;
    updateGeometry();
    emit lengthChanged();
}

void PipeCylinderGeometry::setSides(int sides)
{
    if (m_sides == sides || sides < 3)
        return;
    m_sides = sides;
    updateGeometry();
    emit sidesChanged();
}

void PipeCylinderGeometry::updateGeometry()
{
    clear();

    const int vertexCount = m_sides * 2;
    const int indexCount = m_sides * 6;  // 2 triangles per quad

    // Stride: position (3 floats) + normal (3 floats) = 24 bytes
    const int stride = 6 * sizeof(float);

    QByteArray vertexData;
    vertexData.resize(vertexCount * stride);
    float *vp = reinterpret_cast<float *>(vertexData.data());

    QByteArray indexData;
    indexData.resize(indexCount * sizeof(quint16));
    quint16 *ip = reinterpret_cast<quint16 *>(indexData.data());

    const float halfLength = m_length / 2.0f;

    // Generate vertices (normals point outward from cylinder axis)
    for (int j = 0; j < m_sides; ++j) {
        float phi = (float(j) / m_sides) * 2.0f * M_PI;
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        // Bottom vertex
        int idx = j * 6;
        vp[idx + 0] = m_radius * cosPhi;   // x
        vp[idx + 1] = -halfLength;          // y
        vp[idx + 2] = m_radius * sinPhi;   // z
        vp[idx + 3] = cosPhi;               // nx (outward)
        vp[idx + 4] = 0.0f;                 // ny
        vp[idx + 5] = sinPhi;               // nz (outward)

        // Top vertex
        idx = (m_sides + j) * 6;
        vp[idx + 0] = m_radius * cosPhi;
        vp[idx + 1] = halfLength;
        vp[idx + 2] = m_radius * sinPhi;
        vp[idx + 3] = cosPhi;               // nx (outward)
        vp[idx + 4] = 0.0f;                 // ny
        vp[idx + 5] = sinPhi;               // nz (outward)
    }

    // Generate indices
    int ti = 0;
    for (int j = 0; j < m_sides; ++j) {
        int bottom = j;
        int bottomNext = (j + 1) % m_sides;
        int top = m_sides + j;
        int topNext = m_sides + (j + 1) % m_sides;

        ip[ti++] = bottom;
        ip[ti++] = top;
        ip[ti++] = bottomNext;

        ip[ti++] = bottomNext;
        ip[ti++] = top;
        ip[ti++] = topNext;
    }

    setVertexData(vertexData);
    setIndexData(indexData);
    setStride(stride);
    setBounds(QVector3D(-m_radius, -halfLength, -m_radius),
              QVector3D(m_radius, halfLength, m_radius));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                 0, QQuick3DGeometry::Attribute::U16Type);

    update();
}

// ============================================================================
// PipeElbowGeometry - 90-degree elbow (quarter torus)
// ============================================================================

PipeElbowGeometry::PipeElbowGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    updateGeometry();
}

void PipeElbowGeometry::setPipeRadius(float radius)
{
    if (qFuzzyCompare(m_pipeRadius, radius))
        return;
    m_pipeRadius = radius;
    updateGeometry();
    emit pipeRadiusChanged();
}

void PipeElbowGeometry::setBendRadius(float radius)
{
    if (qFuzzyCompare(m_bendRadius, radius))
        return;
    m_bendRadius = radius;
    updateGeometry();
    emit bendRadiusChanged();
}

void PipeElbowGeometry::setSides(int sides)
{
    if (m_sides == sides || sides < 3)
        return;
    m_sides = sides;
    updateGeometry();
    emit sidesChanged();
}

void PipeElbowGeometry::setSegments(int segments)
{
    if (m_segments == segments || segments < 1)
        return;
    m_segments = segments;
    updateGeometry();
    emit segmentsChanged();
}

void PipeElbowGeometry::updateGeometry()
{
    clear();

    const int numCircles = m_segments + 1;
    const int vertexCount = numCircles * m_sides;
    const int indexCount = m_segments * m_sides * 6;

    const int stride = 6 * sizeof(float);

    QByteArray vertexData;
    vertexData.resize(vertexCount * stride);
    float *vp = reinterpret_cast<float *>(vertexData.data());

    QByteArray indexData;
    indexData.resize(indexCount * sizeof(quint16));
    quint16 *ip = reinterpret_cast<quint16 *>(indexData.data());

    // Generate vertices for quarter torus
    // Elbow goes from -Y direction to +X direction
    int vi = 0;
    for (int i = 0; i < numCircles; ++i) {
        float theta = (float(i) / m_segments) * M_PI / 2.0f;  // 0 to 90 degrees
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (int j = 0; j < m_sides; ++j) {
            float phi = (float(j) / m_sides) * 2.0f * M_PI;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            // Torus parametric equations
            float x = (m_bendRadius + m_pipeRadius * cosPhi) * sinTheta;
            float y = -m_bendRadius + (m_bendRadius + m_pipeRadius * cosPhi) * cosTheta;
            float z = m_pipeRadius * sinPhi;

            int idx = vi * 6;
            vp[idx + 0] = x;
            vp[idx + 1] = y;
            vp[idx + 2] = z;

            // Normal (outward from tube center)
            vp[idx + 3] = cosPhi * sinTheta;
            vp[idx + 4] = cosPhi * cosTheta;
            vp[idx + 5] = sinPhi;

            vi++;
        }
    }

    // Generate indices (winding order for outward-facing normals)
    int ti = 0;
    for (int i = 0; i < m_segments; ++i) {
        for (int j = 0; j < m_sides; ++j) {
            int curr = i * m_sides + j;
            int next = i * m_sides + (j + 1) % m_sides;
            int currUpper = (i + 1) * m_sides + j;
            int nextUpper = (i + 1) * m_sides + (j + 1) % m_sides;

            // Swapped winding order for correct face culling
            ip[ti++] = curr;
            ip[ti++] = next;
            ip[ti++] = currUpper;

            ip[ti++] = next;
            ip[ti++] = nextUpper;
            ip[ti++] = currUpper;
        }
    }

    setVertexData(vertexData);
    setIndexData(indexData);
    setStride(stride);

    float maxExtent = m_bendRadius + m_pipeRadius;
    setBounds(QVector3D(-maxExtent, -maxExtent, -m_pipeRadius),
              QVector3D(maxExtent, maxExtent, m_pipeRadius));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                 0, QQuick3DGeometry::Attribute::U16Type);

    update();
}

// ============================================================================
// PipeCapGeometry - Flat disc end cap
// ============================================================================

PipeCapGeometry::PipeCapGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    updateGeometry();
}

void PipeCapGeometry::setRadius(float radius)
{
    if (qFuzzyCompare(m_radius, radius))
        return;
    m_radius = radius;
    updateGeometry();
    emit radiusChanged();
}

void PipeCapGeometry::setSides(int sides)
{
    if (m_sides == sides || sides < 3)
        return;
    m_sides = sides;
    updateGeometry();
    emit sidesChanged();
}

void PipeCapGeometry::updateGeometry()
{
    clear();

    // Center vertex + rim vertices
    const int vertexCount = 1 + m_sides;
    const int indexCount = m_sides * 3;  // triangles in a fan

    const int stride = 6 * sizeof(float);

    QByteArray vertexData;
    vertexData.resize(vertexCount * stride);
    float *vp = reinterpret_cast<float *>(vertexData.data());

    QByteArray indexData;
    indexData.resize(indexCount * sizeof(quint16));
    quint16 *ip = reinterpret_cast<quint16 *>(indexData.data());

    // Center vertex
    vp[0] = 0.0f;  // x
    vp[1] = 0.0f;  // y
    vp[2] = 0.0f;  // z
    vp[3] = 0.0f;  // nx
    vp[4] = 1.0f;  // ny (facing up along Y)
    vp[5] = 0.0f;  // nz

    // Rim vertices
    for (int j = 0; j < m_sides; ++j) {
        float phi = (float(j) / m_sides) * 2.0f * M_PI;
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        int idx = (1 + j) * 6;
        vp[idx + 0] = m_radius * cosPhi;
        vp[idx + 1] = 0.0f;
        vp[idx + 2] = m_radius * sinPhi;
        vp[idx + 3] = 0.0f;
        vp[idx + 4] = 1.0f;
        vp[idx + 5] = 0.0f;
    }

    // Generate triangle fan indices
    int ti = 0;
    for (int j = 0; j < m_sides; ++j) {
        ip[ti++] = 0;  // center
        ip[ti++] = 1 + j;
        ip[ti++] = 1 + (j + 1) % m_sides;
    }

    setVertexData(vertexData);
    setIndexData(indexData);
    setStride(stride);
    setBounds(QVector3D(-m_radius, 0, -m_radius),
              QVector3D(m_radius, 0, m_radius));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                 0, QQuick3DGeometry::Attribute::U16Type);

    update();
}

// ============================================================================
// PipeSphereGeometry - Sphere with configurable resolution
// ============================================================================

PipeSphereGeometry::PipeSphereGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    updateGeometry();
}

void PipeSphereGeometry::setRadius(float radius)
{
    if (qFuzzyCompare(m_radius, radius))
        return;
    m_radius = radius;
    updateGeometry();
    emit radiusChanged();
}

void PipeSphereGeometry::setSides(int sides)
{
    if (m_sides == sides || sides < 4)
        return;
    m_sides = sides;
    updateGeometry();
    emit sidesChanged();
}

void PipeSphereGeometry::updateGeometry()
{
    clear();

    // Use same number of segments for latitude and longitude
    const int latSegments = m_sides / 2;  // Poles to equator
    const int lonSegments = m_sides;       // Around the equator

    const int vertexCount = (latSegments + 1) * (lonSegments + 1);
    const int indexCount = latSegments * lonSegments * 6;

    const int stride = 6 * sizeof(float);

    QByteArray vertexData;
    vertexData.resize(vertexCount * stride);
    float *vp = reinterpret_cast<float *>(vertexData.data());

    QByteArray indexData;
    indexData.resize(indexCount * sizeof(quint16));
    quint16 *ip = reinterpret_cast<quint16 *>(indexData.data());

    // Generate vertices
    int vi = 0;
    for (int lat = 0; lat <= latSegments; ++lat) {
        float theta = (float(lat) / latSegments) * M_PI;  // 0 to PI (pole to pole)
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= lonSegments; ++lon) {
            float phi = (float(lon) / lonSegments) * 2.0f * M_PI;  // 0 to 2PI
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            // Position on sphere
            float x = m_radius * sinTheta * cosPhi;
            float y = m_radius * cosTheta;
            float z = m_radius * sinTheta * sinPhi;

            // Normal (outward from center)
            float nx = sinTheta * cosPhi;
            float ny = cosTheta;
            float nz = sinTheta * sinPhi;

            int idx = vi * 6;
            vp[idx + 0] = x;
            vp[idx + 1] = y;
            vp[idx + 2] = z;
            vp[idx + 3] = nx;
            vp[idx + 4] = ny;
            vp[idx + 5] = nz;

            vi++;
        }
    }

    // Generate indices (correct winding for outward-facing normals)
    int ti = 0;
    for (int lat = 0; lat < latSegments; ++lat) {
        for (int lon = 0; lon < lonSegments; ++lon) {
            int curr = lat * (lonSegments + 1) + lon;
            int next = curr + 1;
            int below = (lat + 1) * (lonSegments + 1) + lon;
            int belowNext = below + 1;

            // Two triangles per quad - swapped winding order
            ip[ti++] = curr;
            ip[ti++] = next;
            ip[ti++] = below;

            ip[ti++] = next;
            ip[ti++] = belowNext;
            ip[ti++] = below;
        }
    }

    setVertexData(vertexData);
    setIndexData(indexData);
    setStride(stride);
    setBounds(QVector3D(-m_radius, -m_radius, -m_radius),
              QVector3D(m_radius, m_radius, m_radius));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                 0, QQuick3DGeometry::Attribute::U16Type);

    update();
}
