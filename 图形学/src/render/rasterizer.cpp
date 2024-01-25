#include <array>
#include <limits>
#include <tuple>
#include <vector>
#include <algorithm>
#include <cmath>
#include <mutex>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "rasterizer.h"
#include "triangle.h"
#include "render_engine.h"
#include "../utils/math.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using Eigen::Matrix4f;
using Eigen::Vector2i;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::fill;
using std::tuple;

// 给定坐标(x,y)以及三角形的三个顶点坐标，判断(x,y)是否在三角形的内部
bool Rasterizer::inside_triangle(int x, int y, const Vector4f* vertices)
{
    Vector3f v[3];
    for (int i = 0; i < 3; i++) v[i] = {vertices[i].x(), vertices[i].y(), 1.0};

    Vector3f p(float(x), float(y), 1.0f);
    // 计算三个边的叉积
    Vector3f e0 = v[1] - v[0];
    Vector3f e1 = v[2] - v[1];
    Vector3f e2 = v[0] - v[2];

    Vector3f c0 = p - v[0];
    Vector3f c1 = p - v[1];
    Vector3f c2 = p - v[2];

    // 计算叉积
    Vector3f cross0 = e0.cross(c0);
    Vector3f cross1 = e1.cross(c1);
    Vector3f cross2 = e2.cross(c2);
    // 检查叉积的符号
    if ((cross0.z() > 0 && cross1.z() > 0 && cross2.z() > 0) ||
        (cross0.z() < 0 && cross1.z() < 0 && cross2.z() < 0)) {
        return true;  // 点在三角形内
    }
    return false;
}

// 给定坐标(x,y)以及三角形的三个顶点坐标，计算(x,y)对应的重心坐标[alpha, beta, gamma]
tuple<float, float, float> Rasterizer::compute_barycentric_2d(float x, float y, const Vector4f* v)
{
    float c1 = 0.f, c2 = 0.f, c3 = 0.f;
    float x1 = v[0][0];
    float y1 = v[0][1];
    float x2 = v[1][0];
    float y2 = v[1][1];
    float x3 = v[2][0];
    float y3 = v[2][1];
    // 计算重心坐标
    c1 = (-(y3 - y2) * (x - x2) + (x3 - x2) * (y - y2)) / (-(x1-x2)*(y3-y2)+(y1-y2)*(x3-x2));
    c2 = (-(y1 - y3) * (x - x3) + (x1 - x3) * (y - y3)) / (-(x2-x3)*(y1-y3)+(y2-y3)*(x1-x3));
    c3 = 1.0f - c1 - c2;
    return {c1, c2, c3};
}

// 对当前渲染物体的所有三角形面片进行遍历，进行几何变换以及光栅化
void Rasterizer::draw(const std::vector<Triangle>& TriangleList, const GL::Material& material,
                      const std::list<Light>& lights, const Camera& camera)
{
    // iterate over all triangles in TriangleList
    for (const auto& t : TriangleList) {
        Triangle newtriangle = t;
        //(void)newtriangle;
        

        // transform vertex position to world space for interpolating
        //将顶点位置转换到世界空间，用于插值
        std::array<Vector3f, 3> worldspace_pos;
        Eigen::Vector4f wor_pos1, wor_pos2, wor_pos3;
        wor_pos1 = model * newtriangle.vertex[0];
        wor_pos2 = model * newtriangle.vertex[1];
        wor_pos3 = model * newtriangle.vertex[2];
        for (int i = 0; i < 3; i++)worldspace_pos[0][i] = wor_pos1[i]/wor_pos1[3];
        for (int i = 0; i < 3; i++)worldspace_pos[1][i] = wor_pos2[i]/wor_pos2[3];
        for (int i = 0; i < 3; i++)worldspace_pos[2][i] = wor_pos3[i]/wor_pos3[3];

        // Use vetex_shader to transform vertex attributes(position & normals) to
        // view port and set a new triangle
        //使用 vetex_shader 将顶点属性（位置和法线）转换为 视口并设置新的三角形
        VertexShaderPayload payload1,payload2,payload3;
        payload1.normal = t.normal[0];
        payload1.position = t.vertex[0];
        payload2.normal = t.normal[1];
        payload2.position = t.vertex[1];
        payload3.normal = t.normal[2];
        payload3.position = t.vertex[2];
        VertexShaderPayload Payload1, Payload2, Payload3;
        Payload1=vertex_shader(payload1);
        Payload2=vertex_shader(payload2);
        Payload3=vertex_shader(payload3);
        Triangle T;
        T.normal[0] = Payload1.normal;
        T.normal[1] = Payload2.normal;
        T.normal[2] = Payload3.normal;
        T.vertex[0] = Payload1.position;
        T.vertex[1] = Payload2.position;
        T.vertex[2] = Payload3.position;
        // call rasterize_triangle()
        rasterize_triangle(T, worldspace_pos,material, lights, camera);
        
    }
}

// 对顶点的某一属性插值
Vector3f Rasterizer::interpolate(float alpha, float beta, float gamma, const Eigen::Vector3f& vert1,
                                 const Eigen::Vector3f& vert2, const Eigen::Vector3f& vert3,
                                 const Eigen::Vector3f& weight, const float& Z)
{
    Vector3f interpolated_res;
    for (int i = 0; i < 3; i++) {
        interpolated_res[i] = alpha * vert1[i] / weight[0] + beta * vert2[i] / weight[1] +
                              gamma * vert3[i] / weight[2];
    }
    interpolated_res *= Z;
    return interpolated_res;
}

// 对当前三角形进行光栅化
void Rasterizer::rasterize_triangle(const Triangle& t, const std::array<Vector3f, 3>& world_pos,
                                    GL::Material material, const std::list<Light>& lights,
                                    Camera camera)
{
    // 获取三角形的顶点
    Vector4f v0 = t.vertex[0];
    Vector4f v1 = t.vertex[1];
    Vector4f v2 = t.vertex[2];
    // discard all pixels out of the range(including x,y,z)
    //box用来限制范围
    Vector3f minbox;
    Vector3f maxbox;
    for (int i = 0; i < 2; i++) {
        minbox[i] = std::min(std::min(v0[i], v1[i]), v2[i]);
        maxbox[i] = std::max(std::max(v0[i], v1[i]), v2[i]);
    }   
    if (maxbox[0] > width)maxbox[0] = width;
    if (maxbox[1] > height)maxbox[1] = height;
    for (int x = floor(minbox[0]); x <= ceil(maxbox[0]); x++)
    {
        for (int y = floor(minbox[1]); y <= ceil(maxbox[1]); y++)
        {
            if (inside_triangle(x, y, t.vertex))
            {
                // 1. 插值深度
                std::tuple< float, float, float > zhongxin=compute_barycentric_2d(x, y, t.vertex);
                float a = std::get<0>(zhongxin);
                float b = std::get<1>(zhongxin);
                float c = std::get<2>(zhongxin);
                std::array<Vector4f, 3> world_pos1;
                for (int i = 0; i < 3; i++) {
                    world_pos1[i] = Eigen::Vector4f::Zero();
                }
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        world_pos1[i][j] = world_pos[i][j];
                    }
                }
                float w1 = t.vertex[0].w();
                float w2 = t.vertex[1].w();
                float w3 = t.vertex[2].w();
                float depth = 1 / (a / w1 + b / w2 + c / w3);
                int index = get_index(x, y);
                if (depth < depth_buf[index]) {
                    depth_buf[index] = depth;
                    // 2. 插值顶点位置和法线
                    Vector3f weight = { w1,w2,w3 };
                    Vector3f normal = interpolate(a, b, c, t.normal[0], t.normal[1], t.normal[2], weight, depth);
                    Vector3f pos = interpolate(a, b, c, world_pos[0], world_pos[1], world_pos[2], weight, depth);
                    // 3. 片段着色
                    FragmentShaderPayload payload(pos, normal);
                    Vector3f res = phong_fragment_shader(payload, material, lights, camera);
                    // 4. 设置像素
                    Vector2i point = { x,y };
                    Vector3f color = res;
                    set_pixel(point, color);
                }
            }
        }
    }
}

// 初始化整个光栅化渲染器
void Rasterizer::clear(BufferType buff)
{
    if ((buff & BufferType::Color) == BufferType::Color) {
        fill(frame_buf.begin(), frame_buf.end(), RenderEngine::background_color * 255.0f);
    }
    if ((buff & BufferType::Depth) == BufferType::Depth) {
        fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

Rasterizer::Rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

// 给定像素坐标(x,y)，计算frame buffer里对应的index
int Rasterizer::get_index(int x, int y)
{
    return (height - 1 - y) * width + x;
}

// 给定像素点以及fragement shader得到的结果，对frame buffer中对应存储位置进行赋值
void Rasterizer::set_pixel(const Vector2i& point, const Vector3f& res)
{
    int idx        = get_index(point.x(), point.y());
    frame_buf[idx] = res;
}
