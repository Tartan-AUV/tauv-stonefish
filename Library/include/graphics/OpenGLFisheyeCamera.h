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
//  OpenGLFisheyeCamera.h
//  Stonefish
//
//  Created by Codex on 12/03/25.
//

#ifndef __Stonefish_OpenGLFisheyeCamera__
#define __Stonefish_OpenGLFisheyeCamera__

#include "graphics/OpenGLView.h"
#include "graphics/OpenGLDataStructs.h"

namespace sf
{
    class FisheyeCamera;
    class GLSLShader;
    class Ocean;

    //! An OpenGL view rendering an equidistant fisheye from a cubemap.
    class OpenGLFisheyeCamera : public OpenGLView
    {
    public:
        OpenGLFisheyeCamera(glm::vec3 eyePosition, glm::vec3 direction, glm::vec3 cameraUp,
                            GLint x, GLint y, GLint width, GLint height,
                            GLfloat horizontalFovDeg, bool continuousUpdate);
        virtual ~OpenGLFisheyeCamera();

        void setCamera(FisheyeCamera* cam);

        // OpenGLView interface
        void DrawLDR(GLuint destinationFBO, bool updated) override;
        void UpdateTransform() override;
        glm::vec3 GetEyePosition() const override;
        glm::vec3 GetLookingDirection() const override;
        glm::vec3 GetUpDirection() const override;
        glm::mat4 GetProjectionMatrix() const override;
        glm::mat4 GetViewMatrix() const override;
        GLfloat GetFOVX() const override;
        GLfloat GetFOVY() const override;
        GLfloat GetNearClip() const override;
        GLfloat GetFarClip() const override;
        GLint* GetViewport() const override;
        bool needsUpdate() override;
        ViewType getType() const override;

        void SetupCamera(glm::vec3 _eye, glm::vec3 _dir, glm::vec3 _up);
        void SetExposure(GLfloat exposure);
        void ComputeOutput(std::vector<Renderable>& objects, class Ocean* ocean);
        void Update();

        static void Init();
        static void Destroy();

    private:
        void RenderFace(int faceIndex, std::vector<Renderable>& objects, class Ocean* ocean);
        void UpdateViewUBO(const glm::mat4& V, const glm::mat4& P);

        GLuint cubeTex;
        GLuint cubeDepth;
        GLuint cubeFBO;
        GLuint outputTex;
        GLuint displayTex;
        GLuint outputFBO;
        GLuint displayFBO;
        GLuint outputPBO;

        glm::vec3 eye;
        glm::vec3 dir;
        glm::vec3 up;
        glm::vec3 currentDir;
        glm::vec3 currentUp;
        glm::vec3 tempEye;
        glm::vec3 tempDir;
        glm::vec3 tempUp;

        GLfloat fov; // horizontal fisheye FOV in radians, clamped to pi
        GLfloat near;
        GLfloat far;
        GLfloat focal;
        GLfloat exposure;
        GLint cubeSize;
        bool viewportOverrideEnabled;
        GLint viewportOverrideW;
        GLint viewportOverrideH;

        bool _needsUpdate;
        bool newData;
        FisheyeCamera* camera;

        glm::mat4 currentView;
        glm::mat4 currentProj;

        static GLSLShader* warpShader;
        static GLSLShader* flipShader;
    };
}

#endif
