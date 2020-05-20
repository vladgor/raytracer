#include <iostream>
#include <optional>
#include <thread>
#include <fstream>
#include <sstream>

#include "Camera.h"
#include "CheckerTexture.h"
#include "ConstantTexture.h"
#include "Dielectric.h"
#include "Lambertian.h"
#include "Metal.h"
#include "MovingSphere.h"
#include "Sphere.h"
#include "SurfaceGroup.h"

using namespace raytracer;

namespace
{
	constexpr int MAX_DEPTH = 50;
	unsigned int nx = 1920;
	unsigned int ny = nx;
	unsigned int ns = 1;


	Vector3 color(const Ray& ray, const std::shared_ptr<Surface>& world, int depth = 0)
	{
		auto hit = world->hit(ray, 0.001f, std::numeric_limits<float>::max());
		if (hit)
		{
			auto scatter = hit->material()->scatter(ray, hit.value());
			if (scatter && depth < MAX_DEPTH)
			{
				return scatter->attenuation() * color(scatter->ray(), world, depth + 1);
			}
			return Vector3(0.0f, 0.0f, 0.0f);
		}
		auto unitDirection = ray.direction().normalized();
		float t = 0.5f * (1.0f + unitDirection.y());
		return (1.0f - t) * Vector3(1.0f, 1.0f, 1.0f) + t * Vector3(0.5f, 0.7f, 1.0f);
	}


	class BGR
	{
	public:
		BGR(int b, int g, int r) : blue(b), green(g), red(r)
		{
		}

		BGR() = default;

	public:
		int blue{};
		int green{};
		int red{};
	};

	BGR** img;
} // namespace

void StartCasting(Camera& camera, std::shared_ptr<SurfaceGroup> world, int y_start, int y_end)
{
	for (int j = y_end - 1; j >= y_start; --j)
	{
		for (unsigned int i = 0; i < nx; ++i)
		{
			Vector3 c(0.0f, 0.0f, 0.0f);
			for (unsigned int s = 0; s < ns; ++s)
			{
				float u = (static_cast<float>(i) + nextRandomNumber()) / static_cast<float>(nx);
				float v = (static_cast<float>(j) + nextRandomNumber()) / static_cast<float>(ny);
				auto ray = camera.getRay(u, v);
				c = c + color(ray, world);
			}
			c = c / static_cast<float>(ns);
			c = c.sqrt();
			auto ir = static_cast<int>(255.0f * c.x());
			auto ig = static_cast<int>(255.0f * c.y());
			auto ib = static_cast<int>(255.0f * c.z());
			img[j][i] = BGR(ib, ig, ir);
		}

		std::cout << "j=" << j << std::endl;
	}
}

bool SetLaunchParameters(int argc, char* argv[])
{
	auto printHelp = []() -> void
	{
		std::cout << "Usage:" << std::endl
			<< "raytracer.exe size bounces" << std::endl
			<< "Example:" << std::endl
			<< "raytracer.exe 1920 8" << std::endl;
	};

	if (argc < 3)
	{
		printHelp();

		return false;
	}

	try
	{
		if ((std::stringstream(argv[1]) >> nx).fail()) throw std::exception();
		if ((std::stringstream(argv[2]) >> ns).fail()) throw std::exception();
		ny = nx;

		if (nx < 0) throw std::exception();
		if (ns < 0 || ns > 512) throw std::exception();
	}
	catch (std::exception&)
	{
		printHelp();

		return false;
	}

	return true;
}

int main(int argc, char* argv[])
{
	if (!SetLaunchParameters(argc, argv))
	{
		return EXIT_FAILURE;
	}

	Vector3 lookFrom(13.0f, 2.0f, 3.0f);
	Vector3 lookAt(0.0f, 0.0f, 0.0f);
	Vector3 vup(0.0f, 1.0f, 0.0f);
	float vfov = 20.0f;
	float aspect = static_cast<float>(nx) / static_cast<float>(ny);
	float aperture = 0.1f;
	float distToFocus = distance(lookFrom, lookAt);
	Camera camera(lookFrom, lookAt, vup, vfov, aspect, aperture, distToFocus, 0.0f, 1.0f);

	std::vector<std::shared_ptr<Surface>> surfaces;
	surfaces.emplace_back(Sphere::create(
		Vector3(0.0f, -1000.0f, 0.0f), 1000.0f,
		Lambertian::create(CheckerTexture::create(
			ConstantTexture::create(0.2f, 0.3f, 0.1f), ConstantTexture::create(0.9f, 0.9f, 0.9f)))));
	Vector3 deadZone1(-4.0f, 0.2f, 0.0f);
	Vector3 deadZone2(0.0f, 0.2f, 0.0f);
	Vector3 deadZone3(4.0f, 0.2f, 0.0f);
	for (int a = -11; a < 11; ++a)
	{
		for (int b = -11; b < 11; ++b)
		{
			Vector3 center(a + 0.6f * nextRandomNumber(), 0.2f, b + 0.6f * nextRandomNumber());
			if (squaredDistance(center, deadZone1) > 0.81f
				&& squaredDistance(center, deadZone2) > 0.81f
				&& squaredDistance(center, deadZone3) > 0.81f)
			{
				float chance = nextRandomNumber();
				if (chance < 0.75f)
				{
					auto material = Lambertian::create(ConstantTexture::create(
						nextRandomNumber() * nextRandomNumber(),
						nextRandomNumber() * nextRandomNumber(),
						nextRandomNumber() * nextRandomNumber()));
					surfaces.emplace_back(MovingSphere::create(
						center + Vector3(0.0f, nextRandomNumber() * 0.25f, 0.0f),
						center + Vector3(0.0f, nextRandomNumber() * 0.25f, 0.0f),
						0.0f, 1.0f, 0.2f, material));
				}
				else if (chance < 0.90f)
				{
					auto material = Metal::create(ConstantTexture::create(
						                              0.5f * (1.0f + nextRandomNumber()),
						                              0.5f * (1.0f + nextRandomNumber()),
						                              0.5f * nextRandomNumber()),
					                              0.1f * nextRandomNumber());
					surfaces.emplace_back(Sphere::create(center, 0.2f, material));
				}
				else
				{
					auto material = Dielectric::create(1.5f);
					surfaces.emplace_back(Sphere::create(center, 0.2f, material));
				}
			}
		}
	}
	surfaces.emplace_back(Sphere::create(
		Vector3(-4.0f, 1.0f, 0.0f), 1.0f, Lambertian::create(ConstantTexture::create(0.4f, 0.2f, 0.1f))));
	surfaces.emplace_back(Sphere::create(
		Vector3(0.0f, 1.0f, 0.0f), 1.0f, Dielectric::create(1.5f)));
	surfaces.emplace_back(Sphere::create(
		Vector3(4.0f, 1.0f, 0.0f), 1.0f, Metal::create(ConstantTexture::create(0.7f, 0.6f, 0.5f), 0.0f)));
	auto world = SurfaceGroup::create(surfaces);


	img = new BGR*[nx];
	for (unsigned int i = 0; i < nx; i++)
	{
		img[i] = new BGR[ny];
	}

	std::vector<std::thread> threads;
	const auto numberOfThreads = std::thread::hardware_concurrency();

	std::cout << "Number of threads: " << numberOfThreads << std::endl;

	for (unsigned int i = 0; i < numberOfThreads; i++)
	{
		threads.emplace_back(StartCasting,
		                     camera, world, (nx / numberOfThreads) * i, (nx / numberOfThreads) * (i + 1));
	}
	for (unsigned int i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}

	std::ofstream out("pic_dynamic.ppm");
	out << "P3\n" << nx << "\n" << ny << "\n255\n";

	for (int j = ny - 1; j >= 0; j--)
	{
		for (unsigned int i = 0; i < nx; i++)
		{
			out << img[j][i].red << std::endl;
			out << img[j][i].green << std::endl;
			out << img[j][i].blue << std::endl;
		}
	}

	for (unsigned int i = 0; i < nx; i++)
	{
		delete[] img[i];
	}
	delete[] img;

	return EXIT_SUCCESS;
}
