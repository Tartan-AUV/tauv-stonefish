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
//  FisheyeCamera.cpp
//  Stonefish
//
//  Created by Codex on 12/03/25.
//

#include "sensors/vision/FisheyeCamera.h"

#include "core/GraphicalSimulationApp.h"
#include "graphics/OpenGLPipeline.h"
#include "graphics/OpenGLContent.h"
#include "graphics/OpenGLFisheyeCamera.h"

namespace sf
{

FisheyeCamera::FisheyeCamera(std::string uniqueName, unsigned int resolutionX, unsigned int resolutionY, Scalar hFOVDeg, Scalar frequency)
    : Camera(uniqueName, resolutionX, resolutionY, hFOVDeg, frequency)
{
    imageData = nullptr;
    newDataCallback = nullptr;
    glCamera = nullptr;
    exposure = Scalar(0.00015);
}

FisheyeCamera::~FisheyeCamera()
{
    glCamera = nullptr;
}

void FisheyeCamera::InstallNewDataHandler(std::function<void(FisheyeCamera*)> callback)
{
    newDataCallback = callback;
}

void FisheyeCamera::setExposure(Scalar exp)
{
    exposure = exp;
    if(glCamera != nullptr)
        glCamera->SetExposure((GLfloat)exposure);
}

Scalar FisheyeCamera::getExposure() const
{
    return exposure;
}

void* FisheyeCamera::getImageDataPointer(unsigned int index)
{
    return imageData;
}

void FisheyeCamera::NewDataReady(void* data, unsigned int index)
{
    if(newDataCallback != nullptr)
    {
        imageData = (GLubyte*)data;
        newDataCallback(this);
        imageData = nullptr;
    }
}

VisionSensorType FisheyeCamera::getVisionSensorType() const
{
    return VisionSensorType::FISHEYE_CAMERA;
}

OpenGLView* FisheyeCamera::getOpenGLView() const
{
    return glCamera;
}

void FisheyeCamera::InitGraphics()
{
    glCamera = new OpenGLFisheyeCamera(glm::vec3(0,0,0), glm::vec3(0,0,1.f), glm::vec3(0,-1.f,0), 0, 0, resX, resY, (GLfloat)fovH, freq < Scalar(0));
    glCamera->setCamera(this);
    glCamera->SetExposure((GLfloat)exposure);
    UpdateTransform();
    glCamera->UpdateTransform();
    InternalUpdate(0);
    ((GraphicalSimulationApp*)SimulationApp::getApp())->getGLPipeline()->getContent()->AddView(glCamera);
}

void FisheyeCamera::SetupCamera(const Vector3& eye, const Vector3& dir, const Vector3& up)
{
    glm::vec3 eye_ = glm::vec3((GLfloat)eye.x(), (GLfloat)eye.y(), (GLfloat)eye.z());
    glm::vec3 dir_ = glm::vec3((GLfloat)dir.x(), (GLfloat)dir.y(), (GLfloat)dir.z());
    glm::vec3 up_ = glm::vec3((GLfloat)up.x(), (GLfloat)up.y(), (GLfloat)up.z());
    glCamera->SetupCamera(eye_, dir_, up_);
}

void FisheyeCamera::InternalUpdate(Scalar dt)
{
    glCamera->Update();
}

}
