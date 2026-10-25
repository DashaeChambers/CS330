#include <GLFW/glfw3.h>
#include "linmath.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

using namespace std;

const float PI = 3.14159265f;

void processInput(GLFWwindow* window);

enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE };
enum ONOFF { ON, OFF };

class Brick
{
public:
    float red, green, blue;
    float x, y, width;
    int health;
    BRICKTYPE brick_type;
    ONOFF onoff;

    Brick(BRICKTYPE type, float xx, float yy, float ww,
        float rr, float gg, float bb)
        : red(rr), green(gg), blue(bb), x(xx), y(yy),
        width(ww), health(3), brick_type(type), onoff(ON) {
    }

    void Hit()
    {
        if (onoff == OFF || brick_type == REFLECTIVE)
            return;

        --health;
        if (health == 2)
        {
            red = 1.0f; green = 0.75f; blue = 0.1f;
        }
        else if (health == 1)
        {
            red = 1.0f; green = 0.15f; blue = 0.15f;
        }
        else
        {
            onoff = OFF;
        }
    }

    void drawBrick() const
    {
        if (onoff != ON)
            return;

        const float half = width / 2.0f;
        glColor3f(red, green, blue);
        glBegin(GL_QUADS);
        glVertex2f(x - half, y - half);
        glVertex2f(x + half, y - half);
        glVertex2f(x + half, y + half);
        glVertex2f(x - half, y + half);
        glEnd();
    }
};

class Circle
{
public:
    float red, green, blue;
    float radius;
    float x, y;
    float vx, vy;

    Circle(float xx, float yy, float rad, float dx, float dy,
        float r, float g, float b)
        : red(r), green(g), blue(b), radius(rad), x(xx), y(yy),
        vx(dx), vy(dy) {
    }

    void MoveOneStep()
    {
        x += vx;
        y += vy;

        // Reflect off the four window edges.
        if (x + radius >= 1.0f)
        {
            x = 1.0f - radius;
            vx = -static_cast<float>(fabs(vx));
        }
        else if (x - radius <= -1.0f)
        {
            x = -1.0f + radius;
            vx = static_cast<float>(fabs(vx));
        }

        if (y + radius >= 1.0f)
        {
            y = 1.0f - radius;
            vy = -static_cast<float>(fabs(vy));
        }
        else if (y - radius <= -1.0f)
        {
            y = -1.0f + radius;
            vy = static_cast<float>(fabs(vy));
        }
    }

    void DrawCircle() const
    {
        glColor3f(red, green, blue);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);
        for (int i = 0; i <= 360; ++i)
        {
            float angle = i * PI / 180.0f;
            glVertex2f(x + std::cos(angle) * radius,
                y + std::sin(angle) * radius);
        }
        glEnd();
    }

    void ChangeColor()
    {
        red = static_cast<float>(rand()) / RAND_MAX;
        green = static_cast<float>(rand()) / RAND_MAX;
        blue = static_cast<float>(rand()) / RAND_MAX;
    }
};

vector<Circle> world;
vector<Brick> bricks;

bool CircleHitsBrick(const Circle& circle, const Brick& brick)
{
    if (brick.onoff == OFF)
        return false;

    float half = brick.width / 2.0f;
    float nearestX = max(brick.x - half, min(circle.x, brick.x + half));
    float nearestY = max(brick.y - half, min(circle.y, brick.y + half));
    float dx = circle.x - nearestX;
    float dy = circle.y - nearestY;

    return dx * dx + dy * dy <= circle.radius * circle.radius;
}

void HandleBrickCollision(Circle& circle, Brick& brick)
{
    if (!CircleHitsBrick(circle, brick))
        return;

    float half = brick.width / 2.0f;
    float overlapX = circle.radius + half - fabs(circle.x - brick.x);
    float overlapY = circle.radius + half - fabs(circle.y - brick.y);

    // Reflect on the axis with the smallest overlap.
    if (overlapX < overlapY)
    {
        circle.vx = (circle.x < brick.x) ? -fabs(circle.vx) : fabs(circle.vx);
        circle.x += (circle.x < brick.x ? -1.0f : 1.0f) * 0.005f;
    }
    else
    {
        circle.vy = (circle.y < brick.y) ? -fabs(circle.vy) : fabs(circle.vy);
        circle.y += (circle.y < brick.y ? -1.0f : 1.0f) * 0.005f;
    }

    brick.Hit();
}

void HandleCircleCollisions()
{
    for (size_t i = 0; i < world.size(); ++i)
    {
        for (size_t j = i + 1; j < world.size(); ++j)
        {
            float dx = world[j].x - world[i].x;
            float dy = world[j].y - world[i].y;
            float minDistance = world[i].radius + world[j].radius;

            if (dx * dx + dy * dy <= minDistance * minDistance)
            {
                // Swap velocities and change both circles' colors.
                swap(world[i].vx, world[j].vx);
                swap(world[i].vy, world[j].vy);
                world[i].ChangeColor();
                world[j].ChangeColor();

                // Separate overlapping circles to reduce repeated collisions.
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance > 0.0001f)
                {
                    float separation = (minDistance - distance) / 2.0f + 0.001f;
                    float nx = dx / distance;
                    float ny = dy / distance;
                    world[i].x -= nx * separation;
                    world[i].y -= ny * separation;
                    world[j].x += nx * separation;
                    world[j].y += ny * separation;
                }
            }
        }
    }
}

void DrawPaddle(float x, float y, float width, float height)
{
    glColor3f(0.9f, 0.9f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - width / 2.0f, y - height / 2.0f);
    glVertex2f(x + width / 2.0f, y - height / 2.0f);
    glVertex2f(x + width / 2.0f, y + height / 2.0f);
    glVertex2f(x - width / 2.0f, y + height / 2.0f);
    glEnd();
}

int main()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    if (!glfwInit())
        return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(
        640, 640, "CS 330 - Collision Animation", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Build a colorful three-row brick pattern.
    const float brickWidth = 0.22f;
    const float startX = -0.78f;
    const float gapX = 0.26f;
    const float startY = -0.55f;
    const float gapY = 0.24f;

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 7; ++col)
        {
            float red = (row == 0) ? 0.2f : (row == 1 ? 0.2f : 0.9f);
            float green = (row == 0) ? 0.65f : (row == 1 ? 0.4f : 0.25f);
            float blue = (row == 0) ? 1.0f : (row == 1 ? 0.9f : 0.25f);

            bricks.emplace_back(DESTRUCTABLE, startX + col * gapX,
                startY + row * gapY, brickWidth,
                red, green, blue);
        }
    }

    // Reflective obstacles along the sides and top.
    bricks.emplace_back(REFLECTIVE, -0.94f, 0.0f, 0.10f, 0.3f, 0.9f, 0.9f);
    bricks.emplace_back(REFLECTIVE, 0.94f, 0.0f, 0.10f, 0.3f, 0.9f, 0.9f);
    bricks.emplace_back(REFLECTIVE, 0.0f, 0.91f, 0.10f, 0.8f, 0.4f, 1.0f);

    float paddleX = 0.0f;
    const float paddleY = 0.82f;
    const float paddleWidth = 0.30f;
    const float paddleHeight = 0.045f;
    bool spaceWasPressed = false;

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.03f, 0.03f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window);

        // Move the paddle with the arrow keys.
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            paddleX -= 0.025f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            paddleX += 0.025f;

        paddleX = max(-1.0f + paddleWidth / 2.0f,
            min(1.0f - paddleWidth / 2.0f, paddleX));

        // Space creates one circle per key press.
        bool spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceIsPressed && !spaceWasPressed)
        {
            float vx = (rand() % 2 == 0) ? 0.009f : -0.009f;
            world.emplace_back(0.0f, 0.45f, 0.025f, vx, 0.012f,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX);
        }
        spaceWasPressed = spaceIsPressed;

        for (Circle& circle : world)
        {
            circle.MoveOneStep();

            for (Brick& brick : bricks)
                HandleBrickCollision(circle, brick);

            // Bounce upward when a circle hits the paddle.
            bool overlapsPaddle =
                circle.x + circle.radius >= paddleX - paddleWidth / 2.0f &&
                circle.x - circle.radius <= paddleX + paddleWidth / 2.0f &&
                circle.y + circle.radius >= paddleY - paddleHeight / 2.0f &&
                circle.y - circle.radius <= paddleY + paddleHeight / 2.0f;

            if (overlapsPaddle && circle.vy > 0.0f)
            {
                circle.y = paddleY - paddleHeight / 2.0f - circle.radius;
                circle.vy = -fabs(circle.vy);
                circle.vx += (circle.x - paddleX) * 0.02f;
            }
        }

        HandleCircleCollisions();

        for (const Brick& brick : bricks)
            brick.drawBrick();

        DrawPaddle(paddleX, paddleY, paddleWidth, paddleHeight);

        for (const Circle& circle : world)
            circle.DrawCircle();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
