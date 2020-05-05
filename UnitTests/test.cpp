#include "gtest/gtest.h"

#include "../raytracer/Math.h"
#include "../raytracer/Vector3.h"


TEST(CheckMath, checkReflect)
{
    raytracer::Vector3 vec(100, 100, 100);
    raytracer::Vector3 normalized = vec.normalized();

    raytracer::Vector3 reflected = raytracer::reflect(vec, normalized);

    EXPECT_FLOAT_EQ(reflected.x(), -100);
    EXPECT_FLOAT_EQ(reflected.y(), -100);
    EXPECT_FLOAT_EQ(reflected.z(), -100);
}

TEST(CheckMath, checkRefract)
{
    raytracer::Vector3 vec(100, 100, 100);
    raytracer::Vector3 normalized = vec.normalized();

    auto refracted = raytracer::refract(vec, normalized, /*reflectivity=*/0.5);

    EXPECT_FLOAT_EQ(refracted->x(), -0.57735026);
    EXPECT_FLOAT_EQ(refracted->y(), -0.57735026);
    EXPECT_FLOAT_EQ(refracted->z(), -0.57735026);
}

TEST(CheckMath, checkSchlick)
{
    float glare = raytracer::schlick(/*cosine=*/0.5, /*reflectivity=*/1);

    EXPECT_FLOAT_EQ(glare, 0.03125);
}
