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
//  FisheyeCamera.h
//  Stonefish
//
//  Created by Codex on 12/03/25.
//

#ifndef __Stonefish_FisheyeCamera__
#define __Stonefish_FisheyeCamera__

#include <functional>
#include "sensors/vision/Camera.h"
#include "graphics/OpenGLDataStructs.h"

namespace sf
{
    //! A class representing an equidistant fisheye camera.
    class FisheyeCamera : public Camera
    {
    public:
        //! A constructor.
        /*!
         \param uniqueName a name for the sensor
         \param resolutionX the horizontal resolution [pix]
         \param resolutionY the vertical resolution [pix]
         \param horizFOVDeg the circular field of view (max 180 deg)
         \param frequency the sampling frequency of the sensor [Hz] (-1 if updated every simulation step)
         */
        FisheyeCamera(std::string uniqueName, unsigned int resolutionX, unsigned int resolutionY, Scalar horizFOVDeg, Scalar frequency);
        
        //! A destructor.
        virtual ~FisheyeCamera();
        
        //! A method installing a callback for new data.
        /*!
         \param callback the callback to call when a new frame is ready
         */
        void InstallNewDataHandler(std::function<void(FisheyeCamera*)> callback);

        //! A method to set exposure used for HDR-to-LDR conversion.
        /*!
         \param exp exposure scalar (applied before tonemapping)
         */
        void setExposure(Scalar exp);

        //! A method returning the exposure scalar.
        Scalar getExposure() const;
        
        //! A method returning the pointer to the image data.
        /*!
         \return pointer to the image data buffer
         */
        void* getImageDataPointer(unsigned int index = 0) override;
        
        //! A method informing about new data.
        void NewDataReady(void* data, unsigned int index = 0) override;
        
        //! A method returning the type of the vision sensor.
        VisionSensorType getVisionSensorType() const override;
        
        //! A method returning a pointer to the underlying OpenGLView.
        OpenGLView* getOpenGLView() const override;
        
    protected:
        void InitGraphics() override;
        void SetupCamera(const Vector3& eye, const Vector3& dir, const Vector3& up) override;
        void InternalUpdate(Scalar dt) override;
        
    private:
        std::function<void(FisheyeCamera*)> newDataCallback;
        GLubyte* imageData;
        Scalar exposure;
        class OpenGLFisheyeCamera* glCamera;
    };
}

#endif
