#include "../src/rtweekend.h"

#include "../src/camera.h"
#include "../src/color.h"
#include "../src/hittable_list.h"
#include "../src/material.h"
#include "../src/sphere.h"
#include "../src/moving_sphere.h"
#include "../src/aarect.h"
#include "../src/box.h"
#include "../src/constant_medium.h"
#include "../src/bvh.h"
#include "../src/pdf.h"

#include <iostream>
#include <fstream>
#include <windows.h>

color ray_color(const ray& r, const color& background, const hittable& world,
                shared_ptr<hittable_list>& lights, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if(depth <= 0)
        return color(0, 0, 0);

    // If the ray hits nothing, return the background color.
    if(!world.hit(r, 0.001, infinity, rec))
        return background;

    scatter_record srec;
    color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
    if(!rec.mat_ptr->scatter(r, rec, srec))
        return emitted;

    if(srec.is_specular) {
        return srec.attenuation
            * ray_color(srec.specular_ray, background, world, lights, depth-1);
    }

    auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
    mixture_pdf p(light_ptr, srec.pdf_ptr);

    ray scattered = ray(rec.p, p.generate(), r.time());
    auto pdf_val = p.value(scattered.direction());

    return emitted
        + srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
        * ray_color(scattered, background, world, lights, depth-1) / pdf_val;
//
//    ray scattered;
//    color attenuation;
//    color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
//    double pdf_val;
//    color albedo;
//    if(!rec.mat_ptr->scatter(r, rec, albedo, scattered, pdf_val))
//        return emitted;
//
//    auto p0 = make_shared<hittable_pdf>(lights, rec.p);
//    auto p1 = make_shared<cosine_pdf>(rec.normal);
//    mixture_pdf mixed_pdf(p0, p1);
//
//    scattered = ray(rec.p, mixed_pdf.generate(), r.time());
//    pdf_val = mixed_pdf.value(scattered.direction());

//    auto on_light = point3(random_double(213, 343), 554, random_double(227, 332));
//    auto to_light = on_light - rec.p;
//    auto distance_squared = to_light.length_squared();
//    to_light = unit_vector(to_light);
//
//    if(dot(to_light, rec.normal) < 0)
//        return emitted;
//
//    double light_area = (343-213)*(332-227);
//    auto light_cosine = fabs(to_light.y());
//    if(light_cosine < 0.000001)
//        return emitted;
//
//    pdf = distance_squared / (light_cosine * light_area);
//    scattered = ray(rec.p, to_light, r.time());

//    return emitted
//           + albedo * rec.mat_ptr->scattering_pdf(r, rec, scattered)
//             * ray_color(scattered, background, world, lights, depth-1) / pdf_val;
}

hittable_list cornell_box() {
    hittable_list objects;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
    objects.add(make_shared<flip_face>(make_shared<xz_rect>(213, 343, 227, 332, 554, light)));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

    shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);
    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), aluminum);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    objects.add(box1);

//    shared_ptr<hittable> box2 = make_shared<box>(point3(0, 0, 0), point3(165, 165, 165), white);
//    box2 = make_shared<rotate_y>(box2, -18);
//    box2 = make_shared<translate>(box2, vec3(130, 0, 65));
//    objects.add(box2);

    auto glass = make_shared<dielectric>(1.5);
    objects.add(make_shared<sphere>(point3(190, 90, 190), 90, glass));

    return objects;
}

int main() {
    // Image
    const auto aspect_ratio = 1.0 / 1.0;
    const int image_width = 600;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 10;
    const int max_depth = 50;

    // World
    auto world = cornell_box();
    color background(0, 0, 0);
//    shared_ptr<hittable> lights =
//            make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>());
    auto lights = make_shared<hittable_list>();
    lights->add(make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>()));
    lights->add(make_shared<sphere>(point3(190, 90, 190), 90, shared_ptr<material>()));

    // Camera
    point3 lookfrom(278, 278, -800);
    point3 lookat(278, 278, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;
    auto vfov = 40.0;
    auto time0 = 0.0;
    auto time1 = 1.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, time0, time1);

    // Render
    std::ofstream outfile("cornell_box_v8.ppm", std::ios::trunc);
    outfile << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width - 1);
                auto v = (j + random_double()) / (image_height - 1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, background, world, lights, max_depth);
            }
            write_color(outfile, pixel_color, samples_per_pixel);
        }
    }
    std::cerr << "\nDone.\n";
    outfile.close();

    WinExec("D:\\Honeyview\\Honeyview.exe", SW_NORMAL);
}