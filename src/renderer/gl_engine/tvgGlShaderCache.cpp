/*
 * Copyright (c) 2025 the ThorVG project. All rights reserved.

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

#include "tvgGlShaderCache.h"
#include "tvgCompressor.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #ifndef PATH_MAX
        #define PATH_MAX MAX_PATH
    #endif
#else
    #include <dirent.h>
    #include <limits.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

static char cachePath[PATH_MAX];

const char* GlShaderCache::path(const char* vertSrc, const char* fragSrc)
{
#if defined(THORVG_FILE_IO_SUPPORT) && !defined(__EMSCRIPTEN__)
    // Compute hash from shader sources
    auto vertHash = djb2Encode(vertSrc);
    auto fragHash = djb2Encode(fragSrc);
    auto combinedHash = vertHash ^ (fragHash << 1);

    char cacheDir[PATH_MAX];
    auto home = getenv("HOME");
    if (!home) return nullptr;

    #ifdef _WIN32
    // Windows: %LOCALAPPDATA%\thorvg\shaders
    auto appData = getenv("LOCALAPPDATA");
    if (!appData) {
        appData = getenv("APPDATA");
        if (!appData) return nullptr;
    }
    snprintf(cacheDir, PATH_MAX, "%s\\thorvg\\shaders", appData);

    auto attribs = GetFileAttributesA(cacheDir);
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        char thorvgDir[PATH_MAX];
        snprintf(thorvgDir, PATH_MAX, "%s\\thorvg", appData);

        if (!CreateDirectoryA(thorvgDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            return nullptr;
        }

        if (!CreateDirectoryA(cacheDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            return nullptr;
        }
    }
    #elif defined(__ANDROID__)
    // TODO: android cache path
    return nullptr;
    #elif defined(__linux__)
    // TODO: linux cache path
    return nullptr;
    #elif defined(__APPLE__)
    // macOS: ~/Library/Caches/thorvg/shaders
    snprintf(cacheDir, PATH_MAX, "%s/Library/Caches/thorvg/shaders", home);
    struct stat st = {0};
    if (stat(cacheDir, &st) == -1) {
        // Create thorvg directory first
        char thorvgDir[PATH_MAX];
        snprintf(thorvgDir, PATH_MAX, "%s/Library/Caches/thorvg", home);
        mkdir(thorvgDir, 0755);

        // Create shaders subdirectory
        mkdir(cacheDir, 0755);
    }
    #else
    return nullptr;
    #endif

    // Build full cache file path
    #ifdef _WIN32
    snprintf(cachePath, PATH_MAX, "%s\\shader_%08x.bin", cacheDir, combinedHash);
    #else
    snprintf(cachePath, PATH_MAX, "%s/shader_%08x.bin", cacheDir, combinedHash);
    #endif

    return cachePath;
#else
    return nullptr;
#endif
}

/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

uint32_t GlShaderCache::read(const char* vertSrc, const char* fragSrc)
{
    if (!vertSrc || !fragSrc) return 0;

#if defined(THORVG_FILE_IO_SUPPORT) && !defined(__EMSCRIPTEN__)

    if (!glProgramBinarySupport()) {
        return 0;
    }

    // Read from cache if exists
    auto cachePath = path(vertSrc, fragSrc);
    if (!cachePath) return 0;

    auto file = fopen(cachePath, "rb");
    if (!file) return 0;

    GLenum binaryFormat = 0;
    GLsizei length = 0;

    if (fread(&binaryFormat, sizeof(GLenum), 1, file) != 1 || fread(&length, sizeof(GLsizei), 1, file) != 1 ||
        length < 1) {
        TVGLOG("GL_ENGINE", "Failed to read shader cache header: %s", cachePath);
        fclose(file);
        return 0;
    }

    auto binaryData = tvg::malloc<GLubyte*>(length);
    if (!binaryData) {
        fclose(file);
        return 0;
    }

    auto bytesRead = fread(binaryData, 1, length, file);
    fclose(file);

    if (bytesRead != static_cast<size_t>(length)) {
        TVGLOG("GL_ENGINE", "Failed to read shader cache data: %s", cachePath);
        tvg::free(binaryData);
        return 0;
    }

    // Load program binary to shader
    uint32_t progObj = glCreateProgram();
    if (!progObj) {
        TVGLOG("GL_ENGINE", "Failed to load cached shader program object");
        tvg::free(binaryData);
        return 0;
    }

    glProgramBinary(progObj, binaryFormat, binaryData, length);
    tvg::free(binaryData);

    GLint linked = 0;
    glGetProgramiv(progObj, GL_LINK_STATUS, &linked);

    if (!linked) {
        TVGLOG("GL_ENGINE", "Failed to link cached shader program: %s", cachePath);
        glDeleteProgram(progObj);
        return 0;
    }

    TVGLOG("GL_ENGINE", "Shader cache loaded: %s (%d bytes)", cachePath, length);
    return progObj;
#else
    return 0;
#endif
}

void GlShaderCache::write(uint32_t programId, const char* vertSrc, const char* fragSrc)
{
    if (!programId || !vertSrc || !fragSrc) return;

#if defined(THORVG_FILE_IO_SUPPORT) && !defined(__EMSCRIPTEN__)

    if (!glProgramBinarySupport()) {
        return;
    }

    auto cachePath = path(vertSrc, fragSrc);
    if (!cachePath) return;

    GLint numFormats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);
    if (numFormats < 1) {
        TVGLOG("GL_ENGINE", "This GPU does not support any program binary formats");
        return;
    }

    GLint binaryLength = 0;
    glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    if (binaryLength < 1) {
        TVGLOG("GL_ENGINE", "Failed to get program binary length");
        return;
    }

    auto binaryData = tvg::malloc<GLubyte*>(binaryLength);
    if (!binaryData) {
        return;
    }

    GLenum binaryFormat = 0;
    GLsizei length = 0;
    glGetProgramBinary(programId, binaryLength, &length, &binaryFormat, binaryData);

    if (length < 1) {
        TVGLOG("GL_ENGINE", "Failed to retrieve program binary");
        tvg::free(binaryData);
        return;
    }

    // Check if cache file can be written
    FILE* file = fopen(cachePath, "wb");
    if (!file) {
        TVGLOG("GL_ENGINE", "Failed to open cache file for writing: %s", cachePath);
        tvg::free(binaryData);
        return;
    }

    fwrite(&binaryFormat, sizeof(GLenum), 1, file);
    fwrite(&length, sizeof(GLsizei), 1, file);

    auto written = fwrite(binaryData, 1, length, file);
    fclose(file);

    auto success = (written == static_cast<size_t>(length));

    if (success) TVGLOG("GL_ENGINE", "Shader cache written: %s (%d bytes)", cachePath, length);
    else TVGLOG("GL_ENGINE", "Failed to write shader cache: %s", cachePath);

    tvg::free(binaryData);
    return;
#else
    return;
#endif
}
