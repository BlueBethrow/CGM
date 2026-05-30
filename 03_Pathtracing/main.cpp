#include <GLApp.h>
#include <ArcBall.h>
#include <FontRenderer.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Scene.h"
#include "Camera.h"
#include "Pathtracer.h"

class MyGLApp : public GLApp {
public:
	static constexpr uint32_t renderWidth = 600;
	static constexpr uint32_t renderHeight = 600;
	static constexpr uint32_t pathsPerFrame = 1;

	Image image{renderWidth, renderHeight};
	Scene scene;
	Camera camera;
	std::optional<Pathtracer> renderer;
	FontRenderer fontRenderer{"helvetica_neue.bmp", "helvetica_neue.pos"};
	std::shared_ptr<FontEngine> fontEngine{nullptr};
	std::vector<std::pair<Scene, uint32_t>> scenes;
	size_t activeSceneIndex{0};
	std::vector<float> previewData;
	Vec3 previewCenter;
	ArcBall arcBall{Vec2ui{renderWidth, renderHeight}};

	bool mouseDragActive{false};
	bool showPreview{true};

	MyGLApp() : GLApp{renderWidth, renderHeight, 1, "Pathtracing"} {}


	virtual void init() override {
		GL(glDisable(GL_CULL_FACE));
		GL(glEnable(GL_DEPTH_TEST));
		fontEngine = fontRenderer.generateFontEngine();

		camera.setEyePoint(Vec3{0.0f, 0.0f, 2.0f});
		camera.setLookAt(Vec3{0.0f, 0.0f, 0.0f});

		scenes.emplace_back(Scene::genPathTracingScene(), 200);
		scenes.emplace_back(Scene::genCornellBox(), 5000);
		renderer.emplace(8, pathsPerFrame, scenes[activeSceneIndex].second);

    initPathTracing();
	}

	virtual void resize(const Dimensions winDim, const Dimensions fbDim) override
	{
		GLApp::resize(winDim, fbDim);
		arcBall.setWindowSize(Vec2ui{winDim.width, winDim.height});
	}

	Vec3 computePreviewCenter(const std::vector<float>& data) const
	{
		Vec3 center;
		size_t vertexCount = 0;

		for (size_t i = 0; i + 9 < data.size(); i += 10)
		{
			center = center + Vec3{data[i + 0], data[i + 1], data[i + 2]};
			++vertexCount;
		}

		return vertexCount == 0 ? Vec3{} : center / float(vertexCount);
	}

  void initPathTracing() {
		const std::pair<Scene, uint32_t>& activeScene = scenes[activeSceneIndex];
    scene = activeScene.first;
		renderer->setMaxSampleCount(activeScene.second);
    const Vec3 backgroundColor = scene.getBackgroundcolor();
    setBackground(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
    previewData = scene.getTriangleData();
    previewCenter = computePreviewCenter(previewData);
    restartPathTracing();
  }

	void nextScene() {
		activeSceneIndex = (activeSceneIndex + 1) % scenes.size();
		initPathTracing();
	}

	void restartPathTracing()
	{
		renderer->setScene(scene);
		renderer->setCamera(camera);
		renderer->reset();
		showPreview = false;
	}

	void drawPreviewText()
	{
		if (!fontEngine)
			return;

		fontEngine->render("OpenGL Preview", getAspect(), 0.035f, {0.0f, -0.92f}, Alignment::Center, {1.0f, 0.0f, 0.0f, 0.9f});
	}

	void drawSampleText()
	{
		if (!fontEngine || renderer->isFinished())
			return;

		const uint32_t sampleCount = renderer->getSampleCount();
		const uint32_t maxSampleCount = renderer->getMaxSampleCount();

		glDisable(GL_DEPTH_TEST);
		std::stringstream text;
		text << "Ray " << sampleCount << " / " << maxSampleCount;
		fontEngine->render(text.str(), getAspect(), 0.035f, {0.0f, -0.92f},
                       Alignment::Center, {1.0f, 0.0f, 0.0f, 0.9f});
		glEnable(GL_DEPTH_TEST);
	}

	virtual void draw() override {
		if (showPreview)
		{
			setDrawProjection(camera.getViewProjection(getAspect()));
			setDrawTransform(scene.getModel());
			drawTriangles(previewData, TrisDrawType::LIST, false, true);
			drawPreviewText();
			return;
		}

		if (!renderer->isFinished())
			renderer->render(image);

		setDrawProjection(Mat4{});
		setDrawTransform(Mat4{});
		drawImage(image);
		drawSampleText();
	}

	virtual void mouseButton(int button, int state, int mods, double xPosition, double yPosition) override
	{
		if (button != GLENV_MOUSE_BUTTON_LEFT)
			return;

		mouseDragActive = state == GLENV_MOUSE_PRESS;
		if (mouseDragActive)
		{
			arcBall.click(Vec2ui{uint32_t(xPosition), uint32_t(yPosition)});
			showPreview = true;
		}
		else
		{
			restartPathTracing();
		}
	}

	virtual void mouseMove(double xPosition, double yPosition) override
	{
		if (!mouseDragActive)
			return;

		const Mat4 rotation = arcBall.drag(Vec2ui{uint32_t(xPosition), uint32_t(yPosition)}).computeRotation();
		const Vec3 pivot = scene.getModel() * previewCenter;
		const Mat4 rotateAroundPivot = Mat4::translation(pivot) * rotation * Mat4::translation(pivot * -1.0f);
		scene.setModel(rotateAroundPivot * scene.getModel());
		showPreview = true;
	}

	void keyboard(int key, int scancode, int action, int mods) override {
    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_S:
          nextScene();
          break;
      }
    }
	}

} myApp;

#ifdef _WIN32
#include <Windows.h>

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	std::vector<std::string> args = getArgsWindows();
#else
int main(int argc, char** argv) {
	std::vector<std::string> args{ argv + 1, argv + argc };
#endif
	try {
		myApp.run();
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
