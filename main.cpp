#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <cmath>

struct StreamPoint {
    Vector3 pos;
    float baseSpeed;
};

struct Streamline {
    std::vector<StreamPoint> points;
};

Vector3 GetQuadraticBezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t) {
    float u = 1.0f - t;
    Vector3 res;
    res.x = u * u * p0.x + 2.0f * u * t * p1.x + t * t * p2.x;
    res.y = u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y;
    res.z = u * u * p0.z + 2.0f * u * t * p1.z + t * t * p2.z;
    return res;
}

void AddSegment(std::vector<Vector3>& path, const Vector3& start, const Vector3& end, int samples = 20) {
    for (int i = 0; i <= samples; i++) {
        float t = (float)i / (float)samples;
        path.push_back(Vector3Lerp(start, end, t));
    }
}

void AddElbowBezier(std::vector<Vector3>& path, const Vector3& p0, const Vector3& p1, const Vector3& p2, int samples = 25) {
    for (int i = 1; i <= samples; i++) {
        float t = (float)i / (float)samples;
        path.push_back(GetQuadraticBezier(p0, p1, p2, t));
    }
}

Vector3 GetPathPoint(const std::vector<Vector3>& path, float t) {
    if (path.empty()) return Vector3{ 0, 0, 0 };
    if (t <= 0.0f) return path.front();
    if (t >= 1.0f) return path.back();

    float scaledT = t * (path.size() - 1);
    int idx = (int)scaledT;
    float factor = scaledT - idx;

    if (idx >= (int)path.size() - 1) return path.back();
    return Vector3Lerp(path[idx], path[idx + 1], factor);
}

Color VelocityToColor(float normSpeed) {
    float val = Clamp(normSpeed, 0.0f, 1.0f);
    float hue = (1.0f - val) * 240.0f;
    return ColorFromHSV(hue, 0.95f, 0.95f);
}

void BuildSmoothStreamlines(const std::vector<Vector3>& path, std::vector<Streamline>& streams, float maxRadius, int numStreams) {
    const int SAMPLES = 200;

    for (int s = 0; s < numStreams; s++) {
        float angle = ((float)s / (float)numStreams) * 2.0f * PI;
        float rFrac = 0.15f + 0.65f * ((float)(s % 4) / 3.0f);
        float offsetR = maxRadius * rFrac;

        Streamline stream;
        for (int i = 0; i <= SAMPLES; i++) {
            float t = (float)i / (float)SAMPLES;
            Vector3 centerPos = GetPathPoint(path, t);

            float delta = 0.005f;
            Vector3 pNext = GetPathPoint(path, Clamp(t + delta, 0.0f, 1.0f));
            Vector3 dir = Vector3Normalize(Vector3Subtract(pNext, centerPos));
            if (Vector3Length(dir) < 0.001f) dir = Vector3{ 0, 0, 1 };

            Vector3 up = (fabsf(dir.y) > 0.9f) ? Vector3{ 1, 0, 0 } : Vector3{ 0, 1, 0 };
            Vector3 right = Vector3Normalize(Vector3CrossProduct(dir, up));
            Vector3 localUp = Vector3CrossProduct(right, dir);

            Vector3 offset = Vector3Add(
                Vector3Scale(right, cosf(angle) * offsetR),
                Vector3Scale(localUp, sinf(angle) * offsetR)
            );

            Vector3 ptPos = Vector3Add(centerPos, offset);
            float speedFactor = 0.5f + 0.5f * (1.0f - rFrac * rFrac);

            stream.points.push_back(StreamPoint{ ptPos, speedFactor });
        }
        streams.push_back(stream);
    }
}

int main() {
    InitWindow(1280, 720, "CFD Flow Simulation");
    SetTargetFPS(60);

    float scale = 0.001f;
    Vector3 center = Vector3{ 863.61f * scale, 384.08f * scale, 906.13f * scale };

    float orbitAngleX = 0.785f;
    float orbitAngleY = 0.450f;
    float dist = 3.2f;

    Camera camera = { 0 };
    camera.target = center;
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model pipeModel = LoadModel("pipe.obj");

    std::vector<Vector3> trunk;
    AddSegment(trunk, Vector3{101.62f * scale, 214.68f * scale, 1788.64f * scale}, Vector3{101.62f * scale, 214.68f * scale, 1071.10f * scale});
    AddElbowBezier(trunk, Vector3{101.62f * scale, 214.68f * scale, 1071.10f * scale}, Vector3{101.62f * scale, 214.68f * scale, 880.61f * scale}, Vector3{101.64f * scale, 443.27f * scale, 880.61f * scale});
    AddSegment(trunk, Vector3{101.64f * scale, 443.27f * scale, 880.61f * scale}, Vector3{101.64f * scale, 481.40f * scale, 880.61f * scale});
    AddElbowBezier(trunk, Vector3{101.64f * scale, 481.40f * scale, 880.61f * scale}, Vector3{101.64f * scale, 671.93f * scale, 880.61f * scale}, Vector3{292.10f * scale, 671.93f * scale, 880.65f * scale});
    AddSegment(trunk, Vector3{292.10f * scale, 671.93f * scale, 880.65f * scale}, Vector3{914.42f * scale, 671.87f * scale, 857.22f * scale});

    std::vector<Vector3> branchA;
    AddSegment(branchA, Vector3{914.42f * scale, 671.87f * scale, 857.22f * scale}, Vector3{1435.10f * scale, 671.82f * scale, 880.53f * scale});
    AddElbowBezier(branchA, Vector3{1435.10f * scale, 671.82f * scale, 880.53f * scale}, Vector3{1625.68f * scale, 671.87f * scale, 880.53f * scale}, Vector3{1625.68f * scale, 671.93f * scale, 690.10f * scale});
    AddSegment(branchA, Vector3{1625.68f * scale, 671.93f * scale, 690.10f * scale}, Vector3{1625.61f * scale, 671.88f * scale, 23.64f * scale});

    std::vector<Vector3> branchB;
    AddSegment(branchB, Vector3{914.42f * scale, 671.87f * scale, 857.22f * scale}, Vector3{914.42f * scale, 671.87f * scale, 506.20f * scale});
    AddElbowBezier(branchB, Vector3{914.42f * scale, 671.87f * scale, 506.20f * scale}, Vector3{914.42f * scale, 671.87f * scale, 353.83f * scale}, Vector3{914.45f * scale, 519.50f * scale, 353.83f * scale});
    AddSegment(branchB, Vector3{914.45f * scale, 519.50f * scale, 353.83f * scale}, Vector3{914.45f * scale, 235.20f * scale, 353.83f * scale});
    AddElbowBezier(branchB, Vector3{914.45f * scale, 235.20f * scale, 353.83f * scale}, Vector3{914.45f * scale, 82.80f * scale, 353.83f * scale}, Vector3{1066.80f * scale, 82.80f * scale, 353.79f * scale});
    AddSegment(branchB, Vector3{1066.80f * scale, 82.80f * scale, 353.79f * scale}, Vector3{1244.20f * scale, 82.80f * scale, 353.79f * scale});
    AddElbowBezier(branchB, Vector3{1244.20f * scale, 82.80f * scale, 353.79f * scale}, Vector3{1396.59f * scale, 82.80f * scale, 353.79f * scale}, Vector3{1396.59f * scale, 82.83f * scale, 506.20f * scale});
    AddSegment(branchB, Vector3{1396.59f * scale, 82.83f * scale, 506.20f * scale}, Vector3{1396.57f * scale, 82.80f * scale, 660.04f * scale});

    std::vector<Streamline> trunkStreams;
    std::vector<Streamline> branchAStreams;
    std::vector<Streamline> branchBStreams;

    BuildSmoothStreamlines(trunk, trunkStreams, 0.038f, 24);
    BuildSmoothStreamlines(branchA, branchAStreams, 0.038f, 18);
    BuildSmoothStreamlines(branchB, branchBStreams, 0.030f, 18);

    float globalProgress = 0.0f;
    float speedMultiplier = 1.0f;
    bool isAutoRotating = false;
    bool isDarkMode = true;

    Rectangle sliderBox = { 30.0f, 665.0f, 280.0f, 18.0f };
    Rectangle btnRotate = { 30.0f, 595.0f, 180.0f, 32.0f };
    Rectangle btnTheme = { 1070.0f, 20.0f, 180.0f, 32.0f };
    bool isDraggingSlider = false;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mousePos, Rectangle{ sliderBox.x - 10, sliderBox.y - 10, sliderBox.width + 20, sliderBox.height + 20 })) {
                isDraggingSlider = true;
            }
            if (CheckCollisionPointRec(mousePos, btnRotate)) {
                isAutoRotating = !isAutoRotating;
            }
            if (CheckCollisionPointRec(mousePos, btnTheme)) {
                isDarkMode = !isDarkMode;
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            isDraggingSlider = false;
        }

        if (isDraggingSlider) {
            float normX = (mousePos.x - sliderBox.x) / sliderBox.width;
            speedMultiplier = Clamp(normX * 2.0f, 0.2f, 2.0f);
        } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouseDelta = GetMouseDelta();
            orbitAngleX += mouseDelta.x * 0.005f;
            orbitAngleY = Clamp(orbitAngleY + mouseDelta.y * 0.005f, -1.4f, 1.4f);
        }

        dist = Clamp(dist - GetMouseWheelMove() * 0.25f, 1.0f, 8.0f);

        if (isAutoRotating) {
            orbitAngleX += GetFrameTime() * 0.35f;
        }

        camera.position.x = center.x + dist * cosf(orbitAngleY) * cosf(orbitAngleX);
        camera.position.y = center.y + dist * sinf(orbitAngleY);
        camera.position.z = center.z + dist * cosf(orbitAngleY) * sinf(orbitAngleX);

        globalProgress += GetFrameTime() * 0.12f * speedMultiplier;
        if (globalProgress > 1.30f) {
            globalProgress = 0.0f;
        }

        BeginDrawing();
        Color bgColor = isDarkMode ? Color{ 16, 20, 26, 255 } : Color{ 230, 233, 238, 255 };
        ClearBackground(bgColor);

        BeginMode3D(camera);

        float trunkLimit = Clamp(globalProgress / 0.45f, 0.0f, 1.0f);
        for (const auto& stream : trunkStreams) {
            int maxIdx = (int)(trunkLimit * (stream.points.size() - 1));
            for (int i = 0; i < maxIdx; i++) {
                const auto& p1 = stream.points[i];
                const auto& p2 = stream.points[i + 1];
                Color col = VelocityToColor(p1.baseSpeed * (speedMultiplier * 0.5f));
                DrawCylinderEx(p1.pos, p2.pos, 0.0035f, 0.0035f, 6, col);
            }
        }

        if (globalProgress > 0.45f) {
            float branchLimit = Clamp((globalProgress - 0.45f) / 0.55f, 0.0f, 1.0f);
            for (const auto& stream : branchAStreams) {
                int maxIdx = (int)(branchLimit * (stream.points.size() - 1));
                for (int i = 0; i < maxIdx; i++) {
                    const auto& p1 = stream.points[i];
                    const auto& p2 = stream.points[i + 1];
                    Color col = VelocityToColor(p1.baseSpeed * (speedMultiplier * 0.5f));
                    DrawCylinderEx(p1.pos, p2.pos, 0.0035f, 0.0035f, 6, col);
                }
            }

            for (const auto& stream : branchBStreams) {
                int maxIdx = (int)(branchLimit * (stream.points.size() - 1));
                for (int i = 0; i < maxIdx; i++) {
                    const auto& p1 = stream.points[i];
                    const auto& p2 = stream.points[i + 1];
                    Color col = VelocityToColor(p1.baseSpeed * (speedMultiplier * 0.5f));
                    DrawCylinderEx(p1.pos, p2.pos, 0.0035f, 0.0035f, 6, col);
                }
            }
        }

        rlDisableDepthMask();
        Color pipeGlassColor = isDarkMode ? Color{ 170, 195, 220, 45 } : Color{ 70, 110, 150, 55 };
        DrawModelEx(pipeModel, Vector3{ 0, 0, 0 }, Vector3{ 0, 1, 0 }, 0.0f, Vector3{ scale, scale, scale }, pipeGlassColor);
        rlEnableDepthMask();

        DrawGrid(10, 1.0f);
        EndMode3D();

        Color titleColor = isDarkMode ? RAYWHITE : Color{ 25, 35, 45, 255 };
        Color textColor = isDarkMode ? LIGHTGRAY : Color{ 40, 50, 65, 255 };

        DrawText("CFD Velocity Scale (m/s):", 30, 20, 14, textColor);
        for (int i = 0; i < 200; i++) {
            float norm = (float)i / 200.0f;
            Color c = VelocityToColor(1.0f - norm);
            DrawRectangle(30 + i, 42, 1, 16, c);
        }
        DrawText("0 m/s", 30, 64, 12, BLUE);
        DrawText("15 m/s", 115, 64, 12, GREEN);
        DrawText("30 m/s", 200, 64, 12, RED);

        DrawRectangleRec(btnRotate, isAutoRotating ? Color{ 40, 140, 60, 255 } : (isDarkMode ? Color{ 50, 60, 80, 255 } : Color{ 180, 195, 210, 255 }));
        DrawRectangleLines((int)btnRotate.x, (int)btnRotate.y, (int)btnRotate.width, (int)btnRotate.height, isDarkMode ? LIGHTGRAY : DARKGRAY);
        DrawText(isAutoRotating ? "Auto-Rotate: ON" : "Auto-Rotate: OFF", (int)btnRotate.x + 18, (int)btnRotate.y + 8, 14, isDarkMode ? RAYWHITE : Color{ 20, 30, 40, 255 });

        DrawRectangleRec(btnTheme, isDarkMode ? Color{ 50, 60, 80, 255 } : Color{ 200, 215, 230, 255 });
        DrawRectangleLines((int)btnTheme.x, (int)btnTheme.y, (int)btnTheme.width, (int)btnTheme.height, isDarkMode ? LIGHTGRAY : DARKGRAY);
        DrawText(isDarkMode ? "Theme: Dark Mode" : "Theme: Light Mode", (int)btnTheme.x + 16, (int)btnTheme.y + 8, 14, isDarkMode ? RAYWHITE : Color{ 20, 30, 40, 255 });

        DrawText("Flow Speed Slider:", (int)sliderBox.x, (int)sliderBox.y - 20, 13, titleColor);
        DrawRectangleRec(sliderBox, isDarkMode ? Color{ 40, 50, 65, 255 } : Color{ 190, 200, 215, 255 });

        float handleX = sliderBox.x + (speedMultiplier / 2.0f) * sliderBox.width;
        DrawRectangle((int)handleX - 5, (int)sliderBox.y - 3, 10, (int)sliderBox.height + 6, GOLD);

        char speedText[64];
        TextCopy(speedText, TextFormat("%.1f m/s", speedMultiplier * 15.0f));
        DrawText(speedText, (int)sliderBox.x + (int)sliderBox.width + 12, (int)sliderBox.y + 2, 13, GOLD);

        EndDrawing();
    }

    UnloadModel(pipeModel);
    CloseWindow();
    return 0;
}
