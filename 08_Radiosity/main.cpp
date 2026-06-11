#include <GLApp.h>
#include <ArcBall.h>
#include <FontRenderer.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Camera.h"
#include "RadiositySolver.h"
#include "Scene.h"

class MyGLApp : public GLApp {
public:
	static constexpr uint32_t renderWidth = 600;
	static constexpr uint32_t renderHeight = 600;
	static constexpr size_t wallTessellationFactor = 12;
	static constexpr size_t sphereTessellationFactor = 6;
	static constexpr size_t formFactorsPerFrame = 12000;

	enum class SolverStage {
		ComputingFormFactors,
		Ready
	};

	Scene scene;
	Camera camera;
	RadiositySolver solver{160};
	FontRenderer fontRenderer{"helvetica_neue.bmp", "helvetica_neue.pos"};
	std::shared_ptr<FontEngine> fontEngine{nullptr};
	std::vector<RadiosityPatch> patches;
	std::vector<float> progressTriangleData;
	std::vector<float> flatTriangleData;
	std::vector<float> smoothedTriangleData;
	Vec3 previewCenter;
	ArcBall arcBall{Vec2ui{renderWidth, renderHeight}};

	bool mouseDragActive{false};
	bool showSmoothedRadiosity{true};
	SolverStage solverStage{SolverStage::ComputingFormFactors};

	MyGLApp()
		: GLApp{renderWidth, renderHeight, 1, "Radiosity"}
	{
	}

	void init() override {
		GL(glEnable(GL_CULL_FACE));
		GL(glEnable(GL_DEPTH_TEST));
		fontEngine = fontRenderer.generateFontEngine();

		camera.setEyePoint(Vec3{0.0f, 0.0f, 2.2f});
		camera.setLookAt(Vec3{0.0f, 0.0f, -2.4f});
		setLightPos(Vec3{0.0f, 2.0f, 1.5f});

		scene = Scene::genCornellBox(wallTessellationFactor, sphereTessellationFactor);
		prepareRadiosity();

		const Vec3 backgroundColor = scene.getBackgroundcolor();
		setBackground(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
	}

	void resize(const Dimensions winDim, const Dimensions fbDim) override {
		GLApp::resize(winDim, fbDim);
		arcBall.setWindowSize(Vec2ui{winDim.width, winDim.height});
	}

	void prepareRadiosity() {
		patches = scene.createPatches();
		solver.prepareFormFactorComputation(patches);
		progressTriangleData = scene.getFormFactorProgressTriangleData(patches, solver.completedFormFactorRows());
		previewCenter = computePreviewCenter(progressTriangleData, 10);
	}

	void advanceFormFactorComputation() {
		const bool finished = solver.computeFormFactors(scene, patches, formFactorsPerFrame);
		progressTriangleData = scene.getFormFactorProgressTriangleData(patches, solver.completedFormFactorRows());

		if (finished)
			finishRadiosity();
	}

	void finishRadiosity() {
		solver.finishSolve(patches);
		flatTriangleData = scene.getTriangleData(patches);
		smoothedTriangleData = scene.getSmoothedTriangleData(patches);
		previewCenter = computePreviewCenter(flatTriangleData, 7);
		solverStage = SolverStage::Ready;
	}

	Vec3 computePreviewCenter(const std::vector<float>& data, size_t vertexComponentCount) const {
		Vec3 center;
		size_t vertexCount = 0;

		for (size_t i = 0; i + 2 < data.size(); i += vertexComponentCount) {
			center = center + Vec3{data[i + 0], data[i + 1], data[i + 2]};
			++vertexCount;
		}

		return vertexCount == 0 ? Vec3{} : center / float(vertexCount);
	}

	void drawText() {
		if (!fontEngine)
			return;

		std::stringstream text;
		if (solverStage == SolverStage::ComputingFormFactors) {
			const int percent = int(solver.formFactorProgress() * 100.0f);
			text << "Computing form factors: " << percent << "%   rows: "
			     << solver.completedFormFactorRows() << " / " << solver.totalFormFactorRows();
		}
		fontEngine->render(text.str(), getAspect(), 0.03f, {0.0f, -0.92f},
		                   Alignment::Center, {1.0f, 0.0f, 0.0f, 0.9f});
	}

	void draw() override {
		if (solverStage == SolverStage::ComputingFormFactors)
			advanceFormFactorComputation();

		setDrawProjection(camera.getViewProjection(getAspect()));
		setDrawTransform(scene.getModel());
		if (solverStage == SolverStage::ComputingFormFactors)
			drawTriangles(progressTriangleData, TrisDrawType::LIST, false, true);
		else
			drawTriangles(showSmoothedRadiosity ? smoothedTriangleData : flatTriangleData, TrisDrawType::LIST, false, false);
		drawText();
	}

	void mouseButton(int button, int state, int mods, double xPosition, double yPosition) override {
		if (button != GLENV_MOUSE_BUTTON_LEFT)
			return;

		mouseDragActive = state == GLENV_MOUSE_PRESS;
		if (mouseDragActive)
			arcBall.click(Vec2ui{uint32_t(xPosition), uint32_t(yPosition)});
	}

	void mouseMove(double xPosition, double yPosition) override {
		if (!mouseDragActive)
			return;

		const Mat4 rotation = arcBall.drag(Vec2ui{uint32_t(xPosition), uint32_t(yPosition)}).computeRotation();
		const Vec3 pivot = scene.getModel() * previewCenter;
		const Mat4 rotateAroundPivot = Mat4::translation(pivot) * rotation * Mat4::translation(pivot * -1.0f);
		scene.setModel(rotateAroundPivot * scene.getModel());
	}

	void keyboard(int key, int scancode, int action, int mods) override {
		if (action != GLENV_PRESS)
			return;

		switch (key) {
			case GLENV_KEY_R:
				scene.setModel(Mat4{});
				break;
			case GLENV_KEY_S:
				if (solverStage != SolverStage::Ready)
					break;
				showSmoothedRadiosity = !showSmoothedRadiosity;
				break;
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
