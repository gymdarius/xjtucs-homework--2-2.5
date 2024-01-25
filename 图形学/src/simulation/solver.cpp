#include "solver.h"

#include <Eigen/Core>

using Eigen::Vector3f;

// External Force does not changed.

// Function to calculate the derivative of KineticState
KineticState derivative(const KineticState& state)
{
    return KineticState(state.velocity, state.acceleration, Eigen::Vector3f(0, 0, 0));
}

// Function to perform a single Forward Euler step
KineticState forward_euler_step([[maybe_unused]] const KineticState& previous,
                                const KineticState& current)
{
    KineticState waste;
    waste.acceleration = previous.acceleration;
    KineticState cube;
    float temp_time = time_step;
    cube.acceleration = current.acceleration;
    cube.position = current.position + temp_time * current.velocity;
    cube.velocity = current.velocity + cube.acceleration * temp_time;
    return cube;
}
// Function to perform a single Runge-Kutta step
KineticState runge_kutta_step([[maybe_unused]] const KineticState& previous,
                              const KineticState& current)
{
    KineticState K[4];
    for (int i = 0; i < 4; i++) {
        if (i == 0) {
            K[i].acceleration = current.acceleration;
            K[i].position = current.position;
            K[i].velocity = current.velocity;
        }
        else if (i == 3) {
            K[i].acceleration = current.acceleration;
            K[i].position = current.position + time_step * K[i-1].velocity;
            K[i].velocity = current.velocity + time_step * K[i-1].acceleration;
        }
        else {
            K[i].acceleration = current.acceleration;
            K[i].position = current.position + 0.5f * time_step * K[i - 1].velocity;
            K[i].velocity = current.velocity + 0.5f * time_step * K[i - 1].acceleration;
        }
    }
    Vector3f sum1 = { 0,0,0, }, sum2 = { 0,0,0 };
    for (int i = 0; i < 4; i++) {
        if (i == 0 || i == 3) {
            sum1 += K[i].velocity;
            sum2 += K[i].acceleration;
        }
        else {
            sum1 += 2.0f * K[i].velocity;
            sum2 += 2.0f * K[i].acceleration;
        }
    }

    KineticState result;
    result.position = current.position + (time_step / 6.0f) * sum1;
    result.velocity = current.velocity + (time_step / 6.0f) * sum2;
    result.acceleration = current.acceleration;

    return result;
}

// Function to perform a single Backward Euler step
KineticState backward_euler_step([[maybe_unused]] const KineticState& previous,
                                 const KineticState& current)
{
    KineticState waste;
    waste.acceleration = previous.acceleration;
    KineticState cube;
    float temp_time = time_step;
    cube.acceleration = current.acceleration;
    cube.velocity = current.velocity + cube.acceleration * temp_time;
    cube.position = current.position + temp_time * cube.velocity;
    return cube;
}

// Function to perform a single Symplectic Euler step
KineticState symplectic_euler_step(const KineticState& previous, const KineticState& current)
{
    KineticState waste;
    waste.acceleration = previous.acceleration;
    KineticState cube;
    float temp_time = time_step;
    cube.acceleration = current.acceleration;
    cube.velocity = current.velocity + current.acceleration * temp_time;
    cube.position = current.position + temp_time * cube.velocity;
    return cube;
}
