/*    
    This file is a part of Stonefish.

    Stonefish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Stonefish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//
//  OpenGLFisheyeCamera.cpp
//  Stonefish
//
//  Created by Codex on 12/03/25.
//

#include "graphics/OpenGLFisheyeCamera.h"

#include "core/GraphicalSimulationApp.h"
#include "graphics/GLSLShader.h"
#include "graphics/OpenGLContent.h"
#include "graphics/OpenGLPipeline.h"
#include "graphics/OpenGLState.h"
#include "graphics/OpenGLDataStructs.h"
#include "sensors/vision/FisheyeCamera.h"
#include "entities/SolidEntity.h"
#include "entities/forcefields/Ocean.h"
#include "utils/SystemUtil.hpp"

namespace sf
{

GLSLShader* OpenGLFisheyeCamera::warpShader = nullptr;

OpenGLFisheyeCamera::OpenGLFisheyeCamera(glm::vec3 eyePosition, glm::vec3 direction, glm::vec3 cameraUp,
                                         GLint x, GLint y, GLint width, GLint height,
                                         GLfloat horizontalFovDeg, bool continuousUpdate)
: OpenGLView(x, y, width, height)
{
    _needsUpdate = false;
    newData = false;
    camera = nullptr;
    cubeTex = 0;
    cubeDepth = 0;
    cubeFBO = 0;
    outputTex = 0;
    displayTex = 0;
    outputFBO = 0;
    displayFBO = 0;
    outputPBO = 0;
    currentView = glm::mat4(1.f);
    currentProj = glm::mat4(1.f);
    continuous = continuousUpdate;

    near = 0.05f;
    far = 200.f;
    fov = glm::clamp(horizontalFovDeg/180.f * (GLfloat)M_PI, 0.1f, (GLfloat)M_PI);
    focal = 1.0f / (0.5f * fov); // normalized focal: theta = r * fov/2

    cubeSize = std::max(viewportWidth, viewportHeight);

    SetupCamera(eyePosition, direction, cameraUp);
    UpdateTransform();

    // Cubemap target
    glGenTextures(1, &cubeTex);
    OpenGLState::BindTexture(TEX_BASE, GL_TEXTURE_CUBE_MAP, cubeTex);
    for(int face = 0; face < 6; ++face)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA16F, cubeSize, cubeSize, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &cubeDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, cubeDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubeSize, cubeSize);

    glGenFramebuffers(1, &cubeFBO);

    OpenGLState::BindTexture(TEX_BASE, GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Output fisheye
    outputTex = OpenGLContent::GenerateTexture(GL_TEXTURE_2D, glm::uvec3(viewportWidth, viewportHeight, 0),
                                               GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, NULL, FilteringMode::NEAREST, false);
    displayTex = OpenGLContent::GenerateTexture(GL_TEXTURE_2D, glm::uvec3(viewportWidth, viewportHeight, 0),
                                                GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, NULL, FilteringMode::BILINEAR, false);

    std::vector<FBOTexture> fboTextures;
    fboTextures.push_back(FBOTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTex));
    outputFBO = OpenGLContent::GenerateFramebuffer(fboTextures);

    fboTextures.clear();
    fboTextures.push_back(FBOTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, displayTex));
    displayFBO = OpenGLContent::GenerateFramebuffer(fboTextures);

    glGenBuffers(1, &outputPBO);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, outputPBO);
    glBufferData(GL_PIXEL_PACK_BUFFER, viewportWidth * viewportHeight * 3, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

OpenGLFisheyeCamera::~OpenGLFisheyeCamera()
{
    glDeleteTextures(1, &cubeTex);
    glDeleteRenderbuffers(1, &cubeDepth);
    glDeleteFramebuffers(1, &cubeFBO);
    glDeleteTextures(1, &outputTex);
    glDeleteTextures(1, &displayTex);
    glDeleteFramebuffers(1, &outputFBO);
    glDeleteFramebuffers(1, &displayFBO);
    glDeleteBuffers(1, &outputPBO);
}

void OpenGLFisheyeCamera::Init()
{
    if(warpShader == nullptr)
    {
        std::vector<GLSLSource> sources;
        sources.push_back(GLSLSource(GL_VERTEX_SHADER, "saq.vert"));
        sources.push_back(GLSLSource(GL_FRAGMENT_SHADER, "fisheyeWarp.frag"));
        warpShader = new GLSLShader(sources);
        warpShader->AddUniform("texCube", ParameterType::INT);
        warpShader->AddUniform("focal", ParameterType::FLOAT);
        warpShader->AddUniform("maxTheta", ParameterType::FLOAT);
    }
}

void OpenGLFisheyeCamera::Destroy()
{
    if(warpShader != nullptr)
    {
        delete warpShader;
        warpShader = nullptr;
    }
}

void OpenGLFisheyeCamera::setCamera(FisheyeCamera* cam)
{
    camera = cam;
}

ViewType OpenGLFisheyeCamera::getType() const
{
    return ViewType::FISHEYE_CAMERA;
}

bool OpenGLFisheyeCamera::needsUpdate()
{
    if(_needsUpdate)
    {
        _needsUpdate = false;
        return enabled;
    }
    else
        return false;
}

void OpenGLFisheyeCamera::Update()
{
    _needsUpdate = true;
}

void OpenGLFisheyeCamera::SetupCamera(glm::vec3 _eye, glm::vec3 _dir, glm::vec3 _up)
{
    tempDir = _dir;
    tempEye = _eye;
    tempUp = _up;
}

void OpenGLFisheyeCamera::UpdateTransform()
{
    eye = tempEye;
    dir = tempDir;
    up = tempUp;

    // Update UBO with nominal forward-facing view (used for helpers/culling)
    glm::mat4 V = glm::lookAt(eye, eye + dir, up);
    glm::mat4 P = glm::perspective((GLfloat)M_PI_2, 1.f, near, far);
    UpdateViewUBO(V, P);

    if(newData && camera != nullptr)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, outputPBO);
        GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if(src)
        {
            camera->NewDataReady(src);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        newData = false;
    }
}

glm::vec3 OpenGLFisheyeCamera::GetEyePosition() const
{
    return eye;
}

glm::vec3 OpenGLFisheyeCamera::GetLookingDirection() const
{
    return dir;
}

glm::vec3 OpenGLFisheyeCamera::GetUpDirection() const
{
    return up;
}

glm::mat4 OpenGLFisheyeCamera::GetProjectionMatrix() const
{
    return currentProj;
}

glm::mat4 OpenGLFisheyeCamera::GetViewMatrix() const
{
    return currentView;
}

GLfloat OpenGLFisheyeCamera::GetFOVX() const
{
    return (GLfloat)M_PI_2;
}

GLfloat OpenGLFisheyeCamera::GetFOVY() const
{
    return (GLfloat)M_PI_2;
}

GLfloat OpenGLFisheyeCamera::GetNearClip() const
{
    return near;
}

GLfloat OpenGLFisheyeCamera::GetFarClip() const
{
    return far;
}

void OpenGLFisheyeCamera::UpdateViewUBO(const glm::mat4& V, const glm::mat4& P)
{
    currentView = V;
    currentProj = P;
    viewUBOData.VP = P * V;
    viewUBOData.eye = eye;
    ExtractFrustumFromVP(viewUBOData.frustum, viewUBOData.VP);
}

void OpenGLFisheyeCamera::RenderFace(int faceIndex, std::vector<Renderable>& objects, Ocean* ocean)
{
    glm::vec3 faceDir;
    glm::vec3 faceUp;
    switch(faceIndex)
    {
        case 0: faceDir = glm::vec3(1,0,0);  faceUp = glm::vec3(0,-1,0); break; // +X
        case 1: faceDir = glm::vec3(-1,0,0); faceUp = glm::vec3(0,-1,0); break; // -X
        case 2: faceDir = glm::vec3(0,1,0);  faceUp = glm::vec3(0,0,1);  break; // +Y
        case 3: faceDir = glm::vec3(0,-1,0); faceUp = glm::vec3(0,0,-1); break; // -Y
        case 4: faceDir = glm::vec3(0,0,1);  faceUp = glm::vec3(0,-1,0); break; // +Z
        case 5: faceDir = glm::vec3(0,0,-1); faceUp = glm::vec3(0,-1,0); break; // -Z
        default: return;
    }

    glm::mat4 V = glm::lookAt(eye, eye + faceDir, faceUp);
    glm::mat4 P = glm::perspective((GLfloat)M_PI_2, 1.f, near, far);
    UpdateViewUBO(V, P);

    OpenGLContent* content = ((GraphicalSimulationApp*)SimulationApp::getApp())->getGLPipeline()->getContent();
    content->SetCurrentView(this);
    content->SetDrawingMode(DrawingMode::FULL);

    OpenGLState::BindFramebuffer(cubeFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, cubeTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, cubeDepth);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        cError("Fisheye cube face FBO incomplete!");

    OpenGLState::Viewport(0, 0, cubeSize, cubeSize);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw opaque objects
    for(size_t i=0; i<objects.size(); ++i)
    {
        if(objects[i].type == RenderableType::SOLID)
            content->DrawObject(objects[i].objectId, objects[i].lookId, objects[i].model);
        else if(objects[i].type == RenderableType::CABLE)
        {
            auto nodes = objects[i].getDataAsCableNodes();
            content->DrawCable(objects[i].objectId, objects[i].model[0][0], *nodes, objects[i].lookId);
        }
    }

    // Simple particle pass for ocean if underwater
    if(ocean != nullptr && ocean->GetDepth(eye) > 0.f)
    {
        ocean->getOpenGLOcean()->DrawParticles(this);
    }
}

void OpenGLFisheyeCamera::ComputeOutput(std::vector<Renderable>& objects, Ocean* ocean)
{
    // Render cube faces
    for(int face=0; face<6; ++face)
        RenderFace(face, objects, ocean);

    // Warp to fisheye
    OpenGLState::BindFramebuffer(outputFBO);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    OpenGLState::Viewport(0, 0, viewportWidth, viewportHeight);
    warpShader->Use();
    warpShader->SetUniform("texCube", TEX_POSTPROCESS1);
    warpShader->SetUniform("focal", focal);
    warpShader->SetUniform("maxTheta", fov * 0.5f);
    OpenGLState::BindTexture(TEX_POSTPROCESS1, GL_TEXTURE_CUBE_MAP, cubeTex);
    ((GraphicalSimulationApp*)SimulationApp::getApp())->getGLPipeline()->getContent()->DrawSAQ();
    OpenGLState::BindFramebuffer(0);
    OpenGLState::UnbindTexture(TEX_POSTPROCESS1);
    OpenGLState::UseProgram(0);

    // Copy to display texture (vertical flip via quad dimensions)
    OpenGLContent* content = ((GraphicalSimulationApp*)SimulationApp::getApp())->getGLPipeline()->getContent();
    OpenGLState::BindFramebuffer(displayFBO);
    OpenGLState::Viewport(0, 0, viewportWidth, viewportHeight);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    OpenGLState::DisableCullFace();
    content->DrawTexturedQuad(0, viewportHeight, viewportWidth, -viewportHeight, outputTex);
    OpenGLState::EnableCullFace();
    OpenGLState::BindFramebuffer(0);

    // Readback
    if(camera != nullptr)
    {
        OpenGLState::BindTexture(TEX_POSTPROCESS1, GL_TEXTURE_2D, outputTex);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, outputPBO);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        OpenGLState::UnbindTexture(TEX_POSTPROCESS1);
        newData = true;
    }
}

void OpenGLFisheyeCamera::DrawLDR(GLuint destinationFBO, bool updated)
{
    bool display = true;
    unsigned int dispX, dispY;
    GLfloat dispScale;
    if(camera != nullptr)
        display = camera->getDisplayOnScreen(dispX, dispY, dispScale);

    if(display)
    {
        OpenGLContent* content = ((GraphicalSimulationApp*)SimulationApp::getApp())->getGLPipeline()->getContent();
        int windowHeight = ((GraphicalSimulationApp*)SimulationApp::getApp())->getWindowHeight();
        int windowWidth = ((GraphicalSimulationApp*)SimulationApp::getApp())->getWindowWidth();
        OpenGLState::BindFramebuffer(destinationFBO);
        OpenGLState::Viewport(0, 0, windowWidth, windowHeight);
        OpenGLState::DisableCullFace();
        content->DrawTexturedQuad(dispX, dispY+viewportHeight*dispScale, viewportWidth*dispScale, -viewportHeight*dispScale, displayTex);
        OpenGLState::EnableCullFace();
        OpenGLState::BindFramebuffer(0);
    }
}

}
