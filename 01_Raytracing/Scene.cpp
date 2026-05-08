#include "Scene.h"
#include "Material.h"
#include "PointLight.h"
#include "Sphere.h"
#include "Plane.h"

#include <iostream>

void Scene::addObject(std::shared_ptr<const IntersectableObject> object) {
  sceneObjects.push_back(object);
}

void Scene::addLight(std::shared_ptr<const LightSource> ls) {
  lightSources.push_back(ls);
}

Vec3 Scene::getBackgroundcolor() const {
  return backgroundColor;
}

std::optional<Intersection> Scene::intersect(const Ray& ray, bool shadowRay) const {
  std::optional<Intersection> result{};
  for (std::shared_ptr<const IntersectableObject> object : sceneObjects) {
    if (shadowRay && !object->getMaterial().isShadowCaster())
      continue;

    std::optional<Intersection> i = object->intersect(ray);
    if (!i.has_value())
      continue;

    if (!result.has_value() || i.value().getT() < result.value().getT())
      result = i;
  }
  return result;
}

/// <summary>
/// Trace a ray through the scene and compute its color value.
/// </summary>
/// <param name="ray">to trace</param>
/// <param name="IOR">optical density of the material we are currently travelling in</param>
/// <param name="recDepth">recursion depth</param>
/// <returns>final color value computed for this ray</returns>
Vec3 Scene::traceRay(const Ray& ray, float IOR, int recDepth) const {
  // TODO: implement the missing parts of this method according to the 
   
  // no intersection found
  std::optional<Intersection> opt_intersection = intersect(ray, false);
  if (!opt_intersection.has_value())
    return backgroundColor;

  // else intersection found, do recursive ray tracing
  Intersection inter = opt_intersection.value();
  Vec3 interPos = ray.getPosOnRay(inter.getT()); //Intersection Position
  Vec3 offSurfacePos = interPos + inter.getNormal() * OFFSET_EPSILON; //Normal of Surface,where Ray hit
  
  Vec3 localColor{ 0.0f, 0.0f, 0.0f }; //Local Color is used

  // calculation for each light source in the scene Raytracing Color 
  for (std::shared_ptr<const LightSource> ls : lightSources) {
    // Shadow Ray, from Intersection Position to LightSource, used to check if in Shadow or not
    //Check if light source is blocked by any object, if there is an intersection with any object, 
    // it means the light source is blocked by that object, and the intersection with that object is stored in shadowInter
	Ray shadowRay{ offSurfacePos, ls->getDirection(offSurfacePos) }; 
	std::optional<Intersection> shadowInter = intersect(shadowRay, true);

	//Ambient Color comes from Material and LightSource, is calculated for each LightSource, even if in Shadow, because Ambient Light is not blocked by Shadow
    Vec3 ambient = inter.getMaterial().getAmbient() * ls->getAmbient();

	// only calculated when not in Shadow, because if in Shadow the light source is blocked, and only Ambient Light is calculated, Diffuse and Specular Light are blocked by Shadows
    if (!shadowInter.has_value() || shadowInter->getT() > ls->getDistance(offSurfacePos)) {
		//Calculation for Diffuse Color with Light Source to Simulate Scatering of Surface
        float d = Vec3::dot(ls->getDirection(offSurfacePos), inter.getNormal());// Dot Product of LightSource Ray and Normal of Surface
		Vec3 diffuse = inter.getMaterial().getDiffuse() * ls->getDiffuse() * d;//Diffuse in Angle to LightSource
		diffuse = Vec3::clamp(diffuse, 0.0f, 1.0f);//Clamp Diffuse Calculation

		//Calculation for Specular/Reflection of Surface
        Vec3 Rv = Vec3::reflect(ray.getDirection(), inter.getNormal());
         
        float s = pow(std::max(0.0f, Vec3::dot(Rv, ls->getDirection(offSurfacePos))), inter.getMaterial().getExp());
        Vec3 specular = inter.getMaterial().getSpecular() * ls->getSpecular() * s;
        specular = Vec3::clamp(specular, 0.0f, 1.0f);

        localColor = localColor + ambient + diffuse + specular;// calculate local color with ambient for this light source, add diffuse and specular if not in shadow
	}
	else { // Enters when in Shadow, only Ambient Light is calculated
      localColor = localColor + ambient; //Color Calculation
    }
  }

  if (recDepth > 0) {
      Vec3 reflection{ 0.0f, 0.0f, 0.0f }, refraction{ 0.0f, 0.0f, 0.0f }; //Reflection and Refraction Color
      float r, t = 0; //Reflection Ratio, Local Color Ratio, Refraction Ratio
      float l = inter.getMaterial().getLocalRefectivity(); // Local Color Ratio of local illumination

      //Reflection 
      Vec3 Rv = Vec3::reflect(ray.getDirection(), inter.getNormal());// Reflection Ray
      Ray reflectionRay{ offSurfacePos, Rv };// Pos, Direction for Reflection Ray
      reflection = this->traceRay(reflectionRay, IOR, recDepth - 1);

      //Ratio from Local Color, Reflection and Refraction Calculation
      
        // Fresnel equations, with Schlick’s approximation
        if (inter.getMaterial().reflects() && inter.getMaterial().refracts()) {
            float n1 = IOR;
            float n2 = inter.getMaterial().getIndexOfRefraction().value();
            float n = n1 / n2;

            // Refraction 
            Vec3 Tv = Vec3::refract(ray.getDirection(), inter.getNormal(), n).value_or(Vec3{ 0, 0, 0 });
            Ray refractionRay{ offSurfacePos, Tv };// Pos, Direction for Refraction Ray
            refraction = this->traceRay(refractionRay, n2, recDepth - 1);
           
            float cosTheta = std::clamp(-Vec3::dot(ray.getDirection(), inter.getNormal()), 0.0f, 1.0f);
            float r_0 = pow((n1 - n2) / (n1 + n2), 2);
            float R_0 = r_0 + (1 - r_0) * pow(1 - cosTheta, 5);
            float T_0 = 1 - R_0;

            r = (1 - l) * R_0;//Reflection Ratio
            t = (1 - l) * T_0;//Refraction Ratio
        }
        else if (inter.getMaterial().reflects()) { //Only Reflection
            r = 1.0f - l;
            t = 0;
        }
        else if (inter.getMaterial().refracts()) { //Only Refraction
            r = 0;
            t = 1.0f - l;
        }
        else {
            l = 1;//Local Color Ratio
            r = 0;//Reflection Ratio
            t = 0;//Refraction Ratio
        }
      // calculate local color with ambient for this light source, add diffuse and specular if not in shadow, add reflection if reflects
      localColor = l * localColor + r * reflection + t * refraction;
  }
  return localColor;
}

Scene Scene::genSimpleScene() {
  // create an empty scene
  Scene s;

  // and God said, let there be light and there was light
  const auto l = std::make_shared<const PointLight>(Vec3{ 0, 4, -2 },
                                                    Vec3{ 1, 1, 1 },
                                                    Vec3{ 1, 1, 1 },
                                                    Vec3{ 1, 1, 1 });

  // attach the light source to the scene
  s.addLight(l);

  // create the bluish material for the right sphere
  // vec3 are treated as color values in the range [0, 1]
  Material m(Vec3(0.0f, 0.0f, 0.3f),
             Vec3(0.0f, 0.0f, 0.5f),
             Vec3(1.0f, 1.0f, 1.0f),
             8, 0.2f, 1.52f);
  // create a sphere, apply the material above to it and attach it to the scene
  s.addObject(std::make_shared<Sphere>(Vec3{ 0.7f, -0.4f, -2.0f }, 0.9f, m));

  // create the red material and apply it to the left sphere
  m = Material(Vec3{ 0.3f, 0.0f, 0.0f },
               Vec3{ 0.5f, 0.0f, 0.0f },
               Vec3{ 1.0f, 1.0f, 1.0f }, 8, 1);
  s.addObject(std::make_shared<Sphere>(Vec3{ -0.9f, -0.1f, -2.2f }, 0.6f, m));

  // create the yellowish material and apply it to the big sphere in the back
  m = Material(Vec3{ 0.3f, 0.3f, 0.0f },
               Vec3{ 0.7f, 0.7f, 0.0f },
               Vec3{ 1.0f, 1.0f, 0.0f }, 8, 0.3f);
  s.addObject(std::make_shared<Sphere>(Vec3{ 0.0f, 4.0f, -8.0f }, 3.9f, m));

  // create the white ground plane
  m = Material(Vec3{ 0.3f, 0.3f, 0.3f },
               Vec3{ 0.5f, 0.5f, 0.5f },
               Vec3{ 1.0f, 1.0f, 1.0f }, 32, 0.5f);
  s.addObject(std::make_shared<Plane>(Vec3{ 0.0f, 1.0f, 0.0f }, 1.5f, m));

  return s;
}
