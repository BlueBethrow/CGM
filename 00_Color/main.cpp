#include <GLApp.h>
#include <FontRenderer.h>

class MyGLApp : public GLApp {
public:
  Image image{640,480};
  FontRenderer fr{"helvetica_neue.bmp", "helvetica_neue.pos"};
  std::shared_ptr<FontEngine> fe{nullptr};
  std::string text;

  MyGLApp() : GLApp{800,800,1,"Color Picker"} {}

  Vec3 convertPosFromHSVToRGB(float h, float s) {
    // TODO:
    // enter code here that interprets the mouse's
    // x, y position as H ans S (I suggest to set
    // V to 1.0) and converts that tripple to RGB
      
	  float r, g, b = 0.0;
      float v = 1.0;
      if (s == 0) {
          r = v;
          g = v;
          b = v;
      } else {
        float hue = h*360;
        // Clipping
        if (hue == 360.0) {
            hue = 0.0;
        } else {
              // Splitt up in 60deg each (only for switch case ) 
          hue = hue/60;
        }
        long hue_index = (long) trunc(hue);

          float f = hue - hue_index;
          float p = v * (1.0 - s);
          float q = v * (1.0 - (s * f));
          float t = v * (1.0 - (s * (1.0 - f)));

          // Init Normalized r, g, b with 0 - 1.0 ;
          switch (hue_index) {
          case 0: // < 60 Hue
              r = v;
              g = t;
              b = p;
              break;
          case 1: // < 120 Hue
              r = q;
              g = v;
              b = p;
              break;
          case 2: // < 180 Hue
              r = p;
              g = v;
              b = t;
              break;
          case 3: // < 240 Hue
              r = p;
              g = q;
              b = v;
              break;
          case 4: // < 300 Hue
              r = t;
              g = p;
              b = v;
              break; // < 360/0 Hue
          default:
              r = v;
              g = p;
              b = q;
              break;
          }
      }
    return Vec3{r,g,b};
  }
  
  virtual void init() override {
    fe = fr.generateFontEngine();
    for (uint32_t y = 0;y<image.height;++y) {
      for (uint32_t x = 0;x<image.width;++x) {
        const Vec3 rgb = convertPosFromHSVToRGB(float(x)/image.width, float(y)/image.height);
        image.setNormalizedValue(x,y,0,rgb.r); image.setNormalizedValue(x,y,1,rgb.g);
        image.setNormalizedValue(x,y,2,rgb.b); image.setValue(x,y,3,255);
      }
    }
  }
  
  virtual void mouseMove(double xPosition, double yPosition) override {
    Dimensions s = glEnv.getWindowSize();
    if (xPosition < 0 || xPosition > s.width || yPosition < 0 || yPosition > s.height) return;
    const Vec3 hsv{float(360*xPosition/s.width),float(1.0-yPosition/s.height),1.0f};
    const Vec3 rgb = convertPosFromHSVToRGB(float(xPosition/s.width), float(1.0-yPosition/s.height));
    std::stringstream ss; ss << "HSV: " << hsv << "  RGB: " << rgb; text = ss.str();
  }
    
  virtual void draw() override {
    drawImage(image);

    const Dimensions dim{ glEnv.getFramebufferSize() };
    fe->render(text, dim.aspect(), 0.03f, {0,-0.9f}, Alignment::Center, {0,0,0,1});
  }
} myApp;

#ifdef _WIN32
#include <Windows.h>
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
#else
int main(int argc, char** argv) {
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
