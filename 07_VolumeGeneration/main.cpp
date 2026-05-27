#include <GLApp.h>
#include <Tessellation.h>
#include <ArcBall.h>

#include <algorithm>
#include <filesystem>

#include "RescaleAndAddVolume.h"
#include "Clipper.h"
#include "TransferFunction.h"

class Raycaster : public GLApp {
public:
    
  Raycaster(const std::vector<std::string>& args) :
  GLApp(512, 512, 1, "Raycaster", true, false, false, args)
  {
  }

  void updateMatrices() {
    clipBoxSize = Vec3::maxV({0,0,0},Vec3::minV({1,1,1}, clipBoxSize));
    clipBoxShift = Vec3::maxV((Vec3{1,1,1}-clipBoxSize)*-0.5,Vec3::minV((Vec3{1,1,1}-clipBoxSize)*0.5, clipBoxShift));
    clipBox = Mat4::translation(clipBoxShift) * Mat4::scaling(clipBoxSize);
    minBounds = clipBox * Vec3{-0.5,-0.5,-0.5} + 0.5f;
    maxBounds = clipBox * Vec3{ 0.5, 0.5, 0.5} + 0.5f;
    model = Mat4::translation(0,0,zoom) * rotation * Mat4::scaling(volumeExtend);
    const Mat4 modelView = view * model * clipBox;

    modelViewProjection = projection * modelView;
    viewToTexture = Mat4::translation({0.5f,0.5f,0.5f}) * Mat4::inverse(view * model);
    meshNeedsUpdte = true;
  }

  void clipCubeToNearplane() {
    if (!meshNeedsUpdte) return;
    meshNeedsUpdte = false;
    // transpose( inverse( inverse(view*model) ) ) -> transpose(view*model)
    const Vec4 objectSpaceNearPlane{Mat4::transpose(view*model)*Vec4{0,0,1.0f,near+0.01f}};
    const std::vector<float> verts = Clipper::meshPlane(cube.getVertices(),
                                                        objectSpaceNearPlane.xyz,
                                                        objectSpaceNearPlane.w);
    vertCount = verts.size()/3;
    vbCube.setData(verts, 3);
  }

  void generateVolume() {
    const RescaleAndAddVolumeParameters& parameters = parameterSets[currentVolume];
    RescaleAndAddVolume generator{parameters};
    volume = generator.generate();

    voxelCount = Vec3{float(volume.width),float(volume.height),float(volume.depth)};
    volumeExtend = volume.scale*voxelCount/float(volume.maxSize);
    volumeTex.setData(volume.data,
                      uint32_t(volume.width),
                      uint32_t(volume.height),
                      uint32_t(volume.depth), 1);
  }

  void updateTransferFunction() {
    std::vector<Vec4> cloudTransfer(transferFunction.getSize());
    for (size_t i = 0; i < cloudTransfer.size(); ++i) {
      const float x = cloudTransfer.size() > 1
        ? float(i) / float(cloudTransfer.size() - 1)
        : 0.0f;
      const float alpha = std::clamp((x - stepStart) / stepWidth, 0.0f, 1.0f);
      const float smoothAlpha = alpha * alpha * (3.0f - 2.0f * alpha);
      const float opacity = std::pow(smoothAlpha, alphaRisePower) * peakOpacity;
      cloudTransfer[i] = Vec4{0.92f, 0.96f, 1.0f, opacity};
    }
    transferFunction.setData(cloudTransfer);
  }

  virtual void reset() override {
    rotation = Mat4{};
    stepStart = 0.12f;
    stepWidth = 0.16f;
    updateTransferFunction();
    zoom = 0.0f;
    oversampling = 1.0f;
    clipBoxSize = Vec3{1,1,1};
    clipBoxShift = Vec3{0,0,0};
    updateMatrices();
  }

  void setupParameterSets() {
    parameterSets.clear();
    const auto addParameterSet = [this](const RescaleAndAddVolumeParameters& parameters) {
      parameterSets.emplace_back(parameters);
    };

    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     1337, 9, 2.1f, 2.0f, 0.54f, true,
                     0.38f, 0.75f, 0.0f, 1.15f,
                     0.56f, 0.12f, 2.4f, 0.28f, 0.22f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     2491, 7, 1.6f, 2.1f, 0.58f, true,
                     0.34f, 0.82f, 0.0f, 1.00f,
                     0.60f, 0.16f, 1.8f, 0.38f, 0.30f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     4073, 10, 2.8f, 1.9f, 0.50f, true,
                     0.42f, 0.70f, 0.0f, 1.25f,
                     0.55f, 0.10f, 3.2f, 0.45f, 0.36f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     5281, 6, 1.2f, 2.3f, 0.62f, false,
                     0.30f, 0.78f, 0.0f, 1.10f,
                     0.62f, 0.18f, 1.5f, 0.32f, 0.18f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     6899, 8, 3.4f, 2.0f, 0.48f, true,
                     0.46f, 0.88f, 0.0f, 1.35f,
                     0.52f, 0.08f, 4.0f, 0.55f, 0.42f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     8123, 11, 1.9f, 2.0f, 0.57f, true,
                     0.36f, 0.72f, 0.0f, 0.95f,
                     0.58f, 0.14f, 2.8f, 0.50f, 0.28f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     9349, 5, 0.95f, 2.4f, 0.65f, false,
                     0.28f, 0.68f, 0.0f, 1.05f,
                     0.64f, 0.22f, 1.2f, 0.25f, 0.12f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     10657, 9, 2.5f, 2.15f, 0.52f, true,
                     0.40f, 0.80f, 0.0f, 1.45f,
                     0.57f, 0.11f, 3.6f, 0.62f, 0.34f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     11831, 7, 1.45f, 2.0f, 0.60f, true,
                     0.32f, 0.92f, 0.0f, 0.90f,
                     0.61f, 0.20f, 2.1f, 0.42f, 0.26f, false});
    addParameterSet({384, 160, 256, Vec3{1.0f, 1.0f, 1.0f},
                     12799, 12, 3.0f, 1.85f, 0.46f, true,
                     0.44f, 0.76f, 0.0f, 1.20f,
                     0.54f, 0.09f, 4.8f, 0.68f, 0.48f, false});
  }

  virtual void init() override {

    setupParameterSets();
    generateVolume();

    vertCount = cube.getVertices().size();
    cubeArray.bind();
    vbCube.setData(cube.getVertices(), 3);
    cubeArray.connectVertexAttrib(vbCube, program, "vPos", 3);
    setBackground(0.5,0.5,1,1);
  }

  virtual void resize(const Dimensions winDim, const Dimensions fbDim) override {
    GLApp::resize(winDim, fbDim);

    arcball.setWindowSize({winDim.width,winDim.height});
    projection = Mat4{ Mat4::perspective(45, fbDim.aspect(), near, 100) };
    updateMatrices();
  }

  void setupShader() {
    program.enable();
    program.setTexture("volume",volumeTex,0);
    program.setTexture("transfer",transferFunction.getTexture(),1);
    program.setUniform("modelViewProjection", modelViewProjection);
    program.setUniform("clip", clipBox);
    program.setUniform("minBounds", minBounds);
    program.setUniform("maxBounds", maxBounds);
    program.setUniform("voxelCount", voxelCount);
    program.setUniform("cameraPosInTextureSpace", (viewToTexture * Vec4{0,0,0,1}).xyz);
    program.setUniform("oversampling", oversampling);
    program.setUniform("lightDirectionInTextureSpace", Vec3::normalize(lightDirection));
    program.setUniform("lightColor", Vec3{1.0f, 1.0f, 1.0f});
    program.setUniform("ambientColor", Vec3{0.08f, 0.08f, 0.09f});
    program.setUniform("lightIntensity", 1.25f);
    program.setUniform("shadowDensityScale", 12.0f);
    program.setUniform("shadowAlphaThreshold", 0.01f);
    program.setUniform("shadowStepCount", 48);
  }

  void nudgeLightDirection(const Vec3& delta) {
    lightDirection = lightDirection + delta;
    if (lightDirection.sqlength() < 0.001f) {
      lightDirection = Vec3{0.0f, 1.0f, 0.0f};
    }

    std::cout << "Light direction: " << Vec3::normalize(lightDirection) << std::endl;
  }

  virtual void animate(const double animationTime) override {
    clipCubeToNearplane();
  }

  virtual void draw() override {
    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glBlendEquation(GL_FUNC_ADD));
    setupShader();
    cubeArray.bind();
    GL(glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertCount)));

    if (tfEditor) drawTF();
  }

  void drawTF() {
    GL(glDisable(GL_DEPTH_TEST));

    drawRect(Vec4{1.0f,1.0f,1.0f,0.3f},br,tl);
    const std::vector<float> frame{
      br.x, br.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      br.x, tl.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      tl.x, tl.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      tl.x, br.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    };

    const std::vector<Vec4>& data = transferFunction.getData();

    std::vector<float> r(data.size()*7);
    std::vector<float> g(data.size()*7);
    std::vector<float> b(data.size()*7);
    std::vector<float> a(data.size()*7);

    for (size_t i = 0;i<data.size();++i) {
      const float alpha = float(i)/float(data.size());
      const float xPos = br.x + alpha * (tl.x-br.x);
      const Vec4& elem = data[i];

      r[i*7+0] = xPos; r[i*7+1] = br.y + elem.r * (tl.y-br.y); r[i*7+2] = 0;
      r[i*7+3] = 1; r[i*7+4] = 0; r[i*7+5] = 0; r[i*7+6] = 1.0;

      g[i*7+0] = xPos; g[i*7+1] = br.y + elem.g * (tl.y-br.y); g[i*7+2] = 0;
      g[i*7+3] = 0; g[i*7+4] = 1; g[i*7+5] = 0; g[i*7+6] = 1.0;

      b[i*7+0] = xPos; b[i*7+1] = br.y + elem.b * (tl.y-br.y); b[i*7+2] = 0;
      b[i*7+3] = 0; b[i*7+4] = 0; b[i*7+5] = 1; b[i*7+6] = 1.0;

      a[i*7+0] = xPos; a[i*7+1] = br.y + elem.a * (tl.y-br.y); a[i*7+2] = 0;
      a[i*7+3] = 1; a[i*7+4] = 1; a[i*7+5] = 1; a[i*7+6] = 1.0;
    }

    drawLines(r, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::R ? 4 :2);
    drawLines(g, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::G ? 4 :2);
    drawLines(b, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::B ? 4 :2);
    drawLines(a, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::A ? 4 :2);

    drawLines(frame, LineDrawType::LOOP, 4);
  }

  virtual void keyboard(const int key, const int scancode, const int action, const int mods) override {
    std::stringstream ss;
    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_ESCAPE :
          closeWindow();
          break;
        case GLENV_KEY_V:
          currentVolume = (currentVolume + 1) % parameterSets.size();
          generateVolume();
          stepStart = 0.12f;
          stepWidth = 0.16f;
          updateTransferFunction();
          updateMatrices();
          break;
        case GLENV_KEY_Q:
          oversampling *= 2;
          ss << "Raycaster (" << oversampling << " x oversampling)";
          glEnv.setTitle(ss.str());
          break;
        case GLENV_KEY_W:
          oversampling /= 2;
          ss << "Raycaster (" << oversampling << " x oversampling)";
          glEnv.setTitle(ss.str());
          break;
        case GLENV_KEY_T:
          tfEditor = !tfEditor;
          break;
        case GLENV_KEY_A:
          activeChannel = TransferFunction::Channel((int(activeChannel)+1)%4);
          break;
        case GLENV_KEY_P:
          std::cout << transferFunction.encodeForUrl() << std::endl;
          break;
        case GLENV_KEY_R:
          reset();
          break;
        case GLENV_KEY_UP:
          zoom += 0.1f;
          updateMatrices();
          break;
        case GLENV_KEY_DOWN:
          zoom -= 0.1f;
          updateMatrices();
          break;
        case GLENV_KEY_J:
          nudgeLightDirection({-0.1f, 0.0f, 0.0f});
          break;
        case GLENV_KEY_L:
          nudgeLightDirection({0.1f, 0.0f, 0.0f});
          break;
        case GLENV_KEY_I:
          nudgeLightDirection({0.0f, 0.1f, 0.0f});
          break;
        case GLENV_KEY_K:
          nudgeLightDirection({0.0f, -0.1f, 0.0f});
          break;
        case GLENV_KEY_U:
          nudgeLightDirection({0.0f, 0.0f, -0.1f});
          break;
        case GLENV_KEY_O:
          nudgeLightDirection({0.0f, 0.0f, 0.1f});
          break;
      }
    }
    switch (key) {
      case GLENV_KEY_1:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxSize.x += 0.01f;
        else
          clipBoxSize.x -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_2:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxSize.y += 0.01f;
        else
          clipBoxSize.y -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_3:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxSize.z += 0.01f;
        else
          clipBoxSize.z -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_4:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxShift.x += 0.01f;
        else
          clipBoxShift.x -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_5:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxShift.y += 0.01f;
        else
          clipBoxShift.y -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_6:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxShift.z += 0.01f;
        else
          clipBoxShift.z -= 0.01f;
        updateMatrices();
        break;
    }
  }
  
  virtual void mouseMove(const double xPosition, const double yPosition) override {
    if (rightMouseDown) {
      const Dimensions dim = glEnv.getFramebufferSize();
      const double xDelta = xPositionStart - xPosition;
      const double yDelta = yPositionStart - yPosition;
      xPositionStart = xPosition;
      yPositionStart = yPosition;
      
      stepStart += float(xDelta/dim.width);
      stepWidth += float(yDelta/dim.height);
      updateTransferFunction();
    }
    
    if (leftMouseDown) {
      if (tfEditor) {
        const Vec2 bias  = (br)/2.0f+0.5f;
        const Vec2 scale = (tl-br)/2.0f;
        const Dimensions dim = glEnv.getWindowSize();
        const float x = (float(xPosition/dim.width)-bias.x)/scale.x;
        const float y = ((1.0f-float(yPosition/dim.height))-bias.y)/scale.y;
        transferFunction.setValue(x, y, activeChannel);
      } else {
        const Quaternion q = arcball.drag({uint32_t(xPosition),uint32_t(yPosition)});
        arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
        rotation = q.computeRotation() * rotation;
        updateMatrices();
      }
    }
  }
  
  virtual void mouseButton(const int button,
                           const int state,
                           const int mods,
                           const double xPosition,
                           const double yPosition) override {
    if (button == GLENV_MOUSE_BUTTON_RIGHT) {
      rightMouseDown = state == GLENV_MOUSE_PRESS;
      xPositionStart = xPosition;
      yPositionStart = yPosition;
    }
    
    if (button == GLENV_MOUSE_BUTTON_LEFT) {
      leftMouseDown = state == GLENV_MOUSE_PRESS;
      arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
    }
  }

private:
  Tessellation cube{Tessellation::genBrick({0, 0, 0}, {1, 1, 1}).unpack()};
  GLBuffer vbCube{GL_ARRAY_BUFFER};
  GLArray cubeArray;
  GLProgram program{GLProgram::createFromFile("cubeVS.glsl", "cubeFS.glsl","",true, true)};
  size_t vertCount;
  Volume volume;
  TransferFunction transferFunction{256};
  Vec3 voxelCount;
  Vec3 volumeExtend;
  GLTexture3D volumeTex{GL_LINEAR, GL_LINEAR,GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE};

  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  Mat4 clipBox;
  Mat4 modelViewProjection;
  Mat4 viewToTexture;
  Mat4 model;
  Mat4 view{Mat4::lookAt({ 0, 0, 2 }, { 0, 0, 0 }, { 0, 1, 0 })};
  Mat4 projection;

  Vec3 minBounds;
  Vec3 maxBounds;
  Vec3 clipBoxSize{1,1,1};
  Vec3 clipBoxShift{0,0,0};

  bool tfEditor{false};
  const Vec2 br{-0.9f,-0.9f};
  const Vec2 tl{ 0.9f,-0.6f};
  TransferFunction::Channel activeChannel = TransferFunction::Channel::A;

  float oversampling{1.0f};
  float near{0.1f};
  float zoom{0.0f};
  Vec3 lightDirection{0.0f, 1.0f, 0.0f};

  bool meshNeedsUpdte{true};

  std::vector<RescaleAndAddVolumeParameters> parameterSets;
  size_t currentVolume{0};
  float stepStart{0.12f};
  float stepWidth{0.16f};
  float alphaRisePower{0.65f};
  float peakOpacity{0.28f};
  bool leftMouseDown{false};
  bool rightMouseDown{false};
  double xPositionStart{0};
  double yPositionStart{0};

};


#ifdef _WIN32
#include <Windows.h>

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
  std::vector<std::string> args = getArgsWindows();
#else
int main(int argc, char** argv) {
  std::vector<std::string> args{argv + 1, argv + argc};
#endif
  try {
    Raycaster raycaster{args};
    raycaster.run();
  }
  catch (const GLException& e) {
    std::stringstream ss;
    ss << "Insufficient OpenGL Support " << e.what();
#ifndef _WIN32
    std::cerr << ss.str().c_str() << std::endl;
#else
    MessageBoxA(
                NULL,
                ss.str().c_str(),
                "OpenGL Error",
                MB_ICONERROR | MB_OK
                );
#endif
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
