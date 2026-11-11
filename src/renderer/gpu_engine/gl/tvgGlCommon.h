/*
 * Copyright (c) 2020 - 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TVG_GL_COMMON_H_
#define _TVG_GL_COMMON_H_

#include <cassert>
#include <cstddef>
#include "tvgGl.h"
#include "tvgRender.h"
#include "tvgMath.h"
#include "tvgGpuCommon.h"

struct GlStageBuffer;
struct GlRenderTask;
struct GlRenderer;

constexpr float MIN_GL_STROKE_WIDTH = 1.0f;
constexpr float MIN_GL_STROKE_ALPHA = 0.25f;

constexpr uint32_t GL_MAT3_STD140_SIZE = 12; // mat3 is 3 vec4 columns in std140
constexpr uint32_t GL_MAT3_STD140_BYTES = GL_MAT3_STD140_SIZE * sizeof(float);

// Texture allocation and parameter setup must choose a unit explicitly instead
// of inheriting the last draw-time sampler unit. This unit is not reserved:
// sampler 0 reuses it and the state cache restores its draw binding as needed.
constexpr GLenum TVG_GL_TEXTURE_SETUP_UNIT = GL_TEXTURE0;

// All GPU matrices use column major order.
static inline void getMatrix3(const Matrix& mat3, float* matOut)
{
    matOut[0] = mat3.e11; matOut[3] = mat3.e12; matOut[6] = mat3.e13;
    matOut[1] = mat3.e21; matOut[4] = mat3.e22; matOut[7] = mat3.e23;
    matOut[2] = mat3.e31; matOut[5] = mat3.e32; matOut[8] = mat3.e33;
}

// All GPU matrices use column major order. std140 mat3 packs each column into a vec4 stride.
static inline void getMatrix3Std140(const Matrix& mat3, float* matOut)
{
    matOut[0] = mat3.e11; matOut[4] = mat3.e12; matOut[8] = mat3.e13;
    matOut[1] = mat3.e21; matOut[5] = mat3.e22; matOut[9] = mat3.e23;
    matOut[2] = mat3.e31; matOut[6] = mat3.e32; matOut[10] = mat3.e33;
    matOut[3] = 0.0f;     matOut[7] = 0.0f;     matOut[11] = 0.0f;
}

enum class GlStencilMode
{
    None,
    FillNonZero,
    FillEvenOdd,
    Stroke,
};

struct GlGeometryBuffer
{
    Array<float> vertex;
    Array<uint32_t> index;

    void clear()
    {
        vertex.clear();
        index.clear();
    }
};

struct GlGeometry
{
    const Matrix& itransform() const
    {
        if (!mDirty) return imatrix;
        inverse(&matrix, &imatrix);
        mDirty = false;
        return imatrix;
    }

    void transform(const Matrix& m)
    {
        matrix = m;
        mDirty = true;
    }

    bool drawable(RenderUpdateFlag flag) const
    {
        if (flag == RenderUpdateFlag::None) return false;
        auto& buffer = (flag & (RenderUpdateFlag::Stroke | RenderUpdateFlag::GradientStroke)) ? stroke : fill;
        return !buffer.index.empty();
    }

    void prepare(const RenderShape& rshape);
    bool tesselateShape(const RenderShape& rshape, float& multiplier);
    bool tesselateStroke(const RenderShape& rshape);
    bool tesselateThinFill(const RenderPath& path);
    void tesselateImage(const RenderSurface* image);
    void draw(GlRenderTask* task, GlStageBuffer* gpuBuffer, RenderUpdateFlag flag) const;
    GlStencilMode stencilMode(RenderUpdateFlag flag) const;
    RenderRegion bounds() const;

    GlGeometryBuffer fill, stroke;
    Matrix matrix;
    RenderRegion viewport;
    RenderRegion fillBBox, strokeBBox;
    RenderPath optPath;        // optimal transformed path
    RenderPath optStrokePath;  // matching local path using transformed-space tolerance
    float strokeRenderWidth;
    mutable Matrix imatrix;      // inverse matrix
    mutable bool mDirty = true;  // matrix dirty
    FillRule fillRule;
    bool fillWorld : 1;
    bool optPathThin : 1;
    bool optPathSkipFill : 1;
    bool convex : 1;
    bool strokeDirect;
};

struct GlDrawable
{
    virtual ~GlDrawable() = default;
    virtual void destroy(GlRenderer& renderer) = 0;
    bool prepare(RenderUpdateFlag& updateFlags, const Point& viewSize, uint8_t opacity, bool clipper);

    GlGeometry geometry;
    Array<RenderData> clips;
    Point size;  // view size
    uint32_t opacity;
    RenderUpdateFlag flags = RenderUpdateFlag::None;  // a backup flag for deferred update
};

struct GlShape : GlDrawable
{
    void destroy(GlRenderer& renderer) override {}

    const RenderShape* rshape;
    float multiplier = 1.0f;
    struct {
        bool fill = false;
        bool stroke = false;
    } valid;
};

struct GlImage : GlDrawable
{
    void destroy(GlRenderer& renderer) override;

    const RenderSurface* surface;
    GLuint texId = 0;
    uint16_t stamp;  // Tracks TextureMgr::stamp ownership of texId.
    FilterMethod filter;
};

struct GlIntersector
{
    bool intersect(const tvg::Array<tvg::RenderData>& clips, const Point& pt);
    bool intersect(const GlShape* shape, const RenderRegion& region);
    bool intersect(const GlImage* image, const RenderRegion& region);
};

#define MAX_GRADIENT_STOPS 16

struct GlLinearGradientBlock
{
    alignas(16) float nStops[4] = {};
    alignas(16) float startPos[2] = {};
    alignas(8) float stopPos[2] = {};
    alignas(8) float stopPoints[MAX_GRADIENT_STOPS] = {};
    alignas(16) float stopColors[4 * MAX_GRADIENT_STOPS] = {};
    alignas(16) float invTransform[GL_MAT3_STD140_SIZE] = {};
};

// Keep the shared vertex/fragment GradientInfo block in std140 layout.
static_assert(offsetof(GlLinearGradientBlock, startPos) == 16, "Invalid gradient start offset");
static_assert(offsetof(GlLinearGradientBlock, stopPos) == 24, "Invalid gradient end offset");
static_assert(offsetof(GlLinearGradientBlock, stopPoints) == 32, "Invalid gradient stop offset");
static_assert(offsetof(GlLinearGradientBlock, stopColors) == 96, "Invalid gradient color offset");
static_assert(offsetof(GlLinearGradientBlock, invTransform) == 352, "Invalid gradient matrix offset");
static_assert(sizeof(GlLinearGradientBlock) == 400, "Invalid gradient block size");

struct GlRadialGradientBlock
{
    alignas(16) float nStops[4] = {};
    alignas(16) float centerPos[4] = {};
    alignas(16) float radius[2] = {};
    alignas(16) float stopPoints[MAX_GRADIENT_STOPS] = {};
    alignas(16) float stopColors[4 * MAX_GRADIENT_STOPS] = {};
    alignas(16) float invTransform[GL_MAT3_STD140_SIZE] = {};
};

static_assert(offsetof(GlRadialGradientBlock, centerPos) == 16, "Invalid gradient center offset");
static_assert(offsetof(GlRadialGradientBlock, radius) == 32, "Invalid gradient radius offset");
static_assert(offsetof(GlRadialGradientBlock, stopPoints) == 48, "Invalid gradient stop offset");
static_assert(offsetof(GlRadialGradientBlock, stopColors) == 112, "Invalid gradient color offset");
static_assert(offsetof(GlRadialGradientBlock, invTransform) == 368, "Invalid gradient matrix offset");
static_assert(sizeof(GlRadialGradientBlock) == 416, "Invalid gradient block size");

struct GlConicGradientBlock
{
    alignas(16) float nStops[4] = {};
    alignas(16) float stopPoints[MAX_GRADIENT_STOPS] = {};
    alignas(16) float stopColors[4 * MAX_GRADIENT_STOPS] = {};
    alignas(16) float invTransform[GL_MAT3_STD140_SIZE] = {};
};

static_assert(offsetof(GlConicGradientBlock, stopPoints) == 16, "Invalid gradient stop offset");
static_assert(offsetof(GlConicGradientBlock, stopColors) == 80, "Invalid gradient color offset");
static_assert(offsetof(GlConicGradientBlock, invTransform) == 336, "Invalid gradient matrix offset");
static_assert(sizeof(GlConicGradientBlock) == 384, "Invalid gradient block size");

struct GlCompositor : RenderCompositor
{
    RenderRegion bbox;
    CompositionFlag flags;
    BlendMethod blendMethod = BlendMethod::Normal;

    GlCompositor(const RenderRegion& box, CompositionFlag flags) : bbox(box), flags(flags) {}
};

#endif /* _TVG_GL_COMMON_H_ */
