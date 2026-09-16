#include <windows.h>
#include "resource.h"
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// Control IDs
#define ID_WEIGHT_EDIT    2
#define ID_FEET_EDIT      3
#define ID_INCHES_EDIT    4
#define ID_RESULT_LABEL   5
#define ID_CATEGORY_LABEL 6
#define ID_CLEAR_BTN      8
#define ID_METRIC_CHECK   9
#define ID_MIN_BTN        10
#define ID_MAX_BTN        11
#define ID_CLOSE_BTN      12
#define ID_MODE_BTN       13
#define ID_TIMER          1

// Global brushes and fonts
HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
HBRUSH hDarkGrayBrush = CreateSolidBrush(RGB(17, 17, 17));
HBRUSH hHeaderBrush = CreateSolidBrush(RGB(25, 25, 35));
HFONT hCustomFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
    DEFAULT_PITCH | FF_SWISS, L"Helvetica");

HWND hWeightEdit, hFeetEdit, hInchesEdit, hResultLabel, hCategoryLabel;
HWND hWeightLabel, hFeetLabel, hInchesLabel;
HWND hClearBtn, hMetricCheck, hMinBtn, hMaxBtn, hCloseBtn, hModeBtn;

COLORREF currentCategoryColor = RGB(0, 191, 255);
bool g_clearHovered = false;
bool g_minHovered = false;
bool g_maxHovered = false;
bool g_closeHovered = false;
WNDPROC g_oldEditProc = nullptr;

// --- APP STATE ---
bool g_gameMode = false; // Toggles between Calculator and Arcade

// --- FLIGHT SIMULATOR & COMBAT CONTROLS ---
bool g_isDraggingJoystick = false;
float g_joystickOffsetX = 0.0f; 
float g_joystickOffsetY = 0.0f; 

bool g_isDraggingThrottle = false;
float g_throttleValue = 0.5f; 

bool g_isPressingFireBtn = false;

int g_score = 0;
int g_hullIntegrity = 100;
int g_laserCooldown = 0; 

struct Laser {
    float x, y, z;
    bool active;
};
std::vector<Laser> lasers;

// --- NEW: Explosion Data ---
struct Explosion {
    float x, y, z;
    int timer;
    int maxLife;
    float baseRadius;
};
std::vector<Explosion> explosions;
// ------------------------------------------

// --- 3D WARP SPEED ENGINE DATA ---
struct Star { 
    float x, y, z; 
    float speed; 
};

struct CelestialBody { 
    float x, y, z; 
    float speed; 
    int type; 
    float baseRadius; 
};

std::vector<Star> stars;
std::vector<CelestialBody> planets;
const int NUM_STARS = 300;
const int NUM_PLANETS = 35; 
const float MAX_DEPTH = 1000.0f;
// ---------------------------------

void DoCalculate();

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_SETFOCUS) {
        SendMessage(hWnd, EM_SETSEL, 0, -1);
    }
    return CallWindowProc(g_oldEditProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_NCHITTEST: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);
        if (pt.y < 32 && pt.x < 150) return HTCAPTION; 
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    case WM_LBUTTONDOWN: {
        if (!g_gameMode) break; 

        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        float mouseX = (float)LOWORD(lParam);
        float mouseY = (float)HIWORD(lParam);

        // On-Screen Fire Button hit test
        float fx = (float)(width - 155);
        float fy = (float)(height - 70);
        if (sqrt((mouseX - fx) * (mouseX - fx) + (mouseY - fy) * (mouseY - fy)) <= 25.0f) {
            g_isPressingFireBtn = true;
            SetCapture(hwnd);
            break;
        }

        // Joystick hit test
        float jx = (float)(width - 95);
        float jy = (float)(height - 70);
        if (sqrt((mouseX - jx) * (mouseX - jx) + (mouseY - jy) * (mouseY - jy)) <= 45.0f) {
            g_isDraggingJoystick = true;
            SetCapture(hwnd);
            break;
        }

        // Throttle lever hit test
        float tx = (float)(width - 35);
        if (mouseX >= tx - 15 && mouseX <= tx + 15 && mouseY >= height - 125 && mouseY <= height - 15) {
            g_isDraggingThrottle = true;
            SetCapture(hwnd);
            float slotTop = (float)(height - 120);
            float slotHeight = 100.0f;
            g_throttleValue = 1.0f - std::clamp((mouseY - slotTop) / slotHeight, 0.0f, 1.0f);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        if (!g_gameMode) break;

        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        float mouseX = (float)LOWORD(lParam);
        float mouseY = (float)HIWORD(lParam);

        if (g_isDraggingJoystick) {
            float jx = (float)(width - 95);
            float jy = (float)(height - 70);
            float dx = mouseX - jx;
            float dy = mouseY - jy;
            float dist = sqrt(dx * dx + dy * dy);
            float maxRadius = 35.0f; 

            if (dist > maxRadius) {
                dx = (dx / dist) * maxRadius;
                dy = (dy / dist) * maxRadius;
            }

            g_joystickOffsetX = dx / maxRadius;
            g_joystickOffsetY = dy / maxRadius;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (g_isDraggingThrottle) {
            float slotTop = (float)(height - 120);
            float slotHeight = 100.0f;
            g_throttleValue = 1.0f - std::clamp((mouseY - slotTop) / slotHeight, 0.0f, 1.0f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (g_isDraggingJoystick || g_isDraggingThrottle || g_isPressingFireBtn) {
            g_isDraggingJoystick = false;
            g_isDraggingThrottle = false;
            g_isPressingFireBtn = false;
            g_joystickOffsetX = 0.0f; 
            g_joystickOffsetY = 0.0f;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }

    case WM_TIMER: {
        float speedMultiplier = g_gameMode ? (g_throttleValue * 2.5f) : 0.5f; 

        // Move Stars (Always active)
        for (auto& star : stars) {
            star.z -= star.speed * speedMultiplier;
            if (g_gameMode) {
                star.x -= g_joystickOffsetX * 8.0f; 
                star.y -= g_joystickOffsetY * 8.0f;
            }

            if (star.z < 1.0f) {
                star.z = MAX_DEPTH;
                star.x = (float)((rand() % 2000) - 1000);
                star.y = (float)((rand() % 2000) - 1000);
            }
        }

        // Only process game logic if Arcade Mode is active
        if (g_gameMode) {
            if (g_laserCooldown > 0) g_laserCooldown--;
            
            if (((GetAsyncKeyState(VK_SPACE) & 0x8000) || g_isPressingFireBtn) && g_laserCooldown == 0) {
                lasers.push_back({ -2.5f, 3.0f, 10.0f, true }); 
                lasers.push_back({  2.5f, 3.0f, 10.0f, true }); 
                g_laserCooldown = 5; 
            }

            for (auto& l : lasers) {
                if (!l.active) continue;
                l.z += 40.0f * speedMultiplier; 
                l.x -= g_joystickOffsetX * 6.0f; 
                l.y -= g_joystickOffsetY * 6.0f;
                if (l.z > MAX_DEPTH) l.active = false;

                for (auto& p : planets) {
                    if (l.z > p.z - 300.0f && l.z < p.z + 300.0f) { 
                        float dist = sqrt((p.x - l.x) * (p.x - l.x) + (p.y - l.y) * (p.y - l.y));
                        if (dist < p.baseRadius * 3.5f) { 
                            g_score += (p.type == 9) ? 500 : 100; 
                            explosions.push_back({ p.x, p.y, p.z, 12, 12, p.baseRadius * 3.0f }); // 🔥 BOOM!
                            p.z = MAX_DEPTH; 
                            p.x = (float)((rand() % 4000) - 2000);
                            p.y = (float)((rand() % 4000) - 2000);
                            l.active = false;
                            break;
                        }
                    }
                }
            }
            lasers.erase(std::remove_if(lasers.begin(), lasers.end(), [](const Laser& l) { return !l.active; }), lasers.end());

            for (auto& p : planets) {
                p.z -= p.speed * speedMultiplier;
                p.x -= g_joystickOffsetX * 6.0f;
                p.y -= g_joystickOffsetY * 6.0f;

                if (p.z < 10.0f && p.z > 0.0f) {
                    if (abs(p.x) < p.baseRadius * 2.5f && abs(p.y) < p.baseRadius * 2.5f) {
                        g_hullIntegrity -= 15; 
                        explosions.push_back({ p.x, p.y, 5.0f, 15, 15, 150.0f }); // 🔥 COCKPIT CRASH!
                        p.z = -1.0f; 
                        
                        if (g_hullIntegrity <= 0) {
                            g_hullIntegrity = 100; 
                            g_score = 0;
                        }
                    }
                }

                if (p.z < 1.0f) {
                    p.z = MAX_DEPTH;
                    p.x = (float)((rand() % 4000) - 2000);
                    p.y = (float)((rand() % 4000) - 2000);
                    p.type = rand() % 10; 
                }
            }

            // Animate and clean up old explosions
            for (auto& exp : explosions) {
                exp.timer--;
                exp.z -= 40.0f * speedMultiplier; 
                exp.x -= g_joystickOffsetX * 6.0f;
                exp.y -= g_joystickOffsetY * 6.0f;
            }
            explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& e) { return e.timer <= 0; }), explosions.end());
        }

        POINT pt;
        GetCursorPos(&pt);
        RECT rClear, rMin, rMax, rClose;
        GetWindowRect(hClearBtn, &rClear);
        GetWindowRect(hMinBtn, &rMin);
        GetWindowRect(hMaxBtn, &rMax);
        GetWindowRect(hCloseBtn, &rClose);

        bool overClear = PtInRect(&rClear, pt);
        bool overMin = PtInRect(&rMin, pt);
        bool overMax = PtInRect(&rMax, pt);
        bool overClose = PtInRect(&rClose, pt);

        if (overClear != g_clearHovered) { g_clearHovered = overClear; InvalidateRect(hClearBtn, NULL, TRUE); }
        if (overMin != g_minHovered) { g_minHovered = overMin; InvalidateRect(hMinBtn, NULL, TRUE); }
        if (overMax != g_maxHovered) { g_maxHovered = overMax; InvalidateRect(hMaxBtn, NULL, TRUE); }
        if (overClose != g_closeHovered) { g_closeHovered = overClose; InvalidateRect(hCloseBtn, NULL, TRUE); }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        float cx = width / 2.0f;
        float cy = height / 2.0f;
        float fov = 300.0f;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        FillRect(memDC, &rect, hBlackBrush);

        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH hGrayBrush = CreateSolidBrush(RGB(150, 150, 150));
        HBRUSH hDimBrush = CreateSolidBrush(RGB(50, 50, 50));

        for (auto& star : stars) {
            float screen_x = cx + (star.x / star.z) * fov;
            float screen_y = cy + (star.y / star.z) * fov;

            if (screen_x >= 0 && screen_x < width && screen_y >= 0 && screen_y < height) {
                int size = 1;
                HBRUSH currentBrush = hDimBrush;

                if (star.z < MAX_DEPTH / 4.0f) { size = 3; currentBrush = hWhiteBrush; }
                else if (star.z < MAX_DEPTH / 1.5f) { size = 2; currentBrush = hGrayBrush; }

                RECT sRect = { (int)screen_x, (int)screen_y, (int)screen_x + size, (int)screen_y + size };
                FillRect(memDC, &sRect, currentBrush);
            }
        }

        HPEN hNullPen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
        HPEN hSaturnRingPen = CreatePen(PS_SOLID, 3, RGB(200, 160, 110));
        HPEN hOldPen = (HPEN)SelectObject(memDC, hNullPen);

        // Only draw Planets, Aliens, Lasers, Explosions, and Joystick stuff if Game Mode is ON
        if (g_gameMode) {

            std::sort(planets.begin(), planets.end(), [](const CelestialBody& a, const CelestialBody& b) {
                return a.z > b.z;
            });

            HBRUSH hEarthBrush = CreateSolidBrush(RGB(30, 144, 255));
            HBRUSH hEarthLandBrush = CreateSolidBrush(RGB(34, 139, 34));
            HBRUSH hMarsBrush = CreateSolidBrush(RGB(178, 34, 34));
            HBRUSH hMoonBrush = CreateSolidBrush(RGB(180, 180, 180));
            HBRUSH hCraterBrush = CreateSolidBrush(RGB(120, 120, 120));
            HBRUSH hAsteroidBrush = CreateSolidBrush(RGB(110, 65, 35));
            HBRUSH hVenusBrush = CreateSolidBrush(RGB(245, 222, 179));
            HBRUSH hJupiterBrush = CreateSolidBrush(RGB(210, 140, 70));
            HBRUSH hJupiterStripe1 = CreateSolidBrush(RGB(165, 90, 40));
            HBRUSH hJupiterStripe2 = CreateSolidBrush(RGB(130, 70, 30));
            HBRUSH hSaturnBrush = CreateSolidBrush(RGB(230, 190, 130));
            HBRUSH hSunBrush = CreateSolidBrush(RGB(255, 220, 50));
            HBRUSH hSunCorona = CreateSolidBrush(RGB(255, 100, 0));
            HBRUSH hShadowBrush = CreateSolidBrush(RGB(10, 10, 10));
            HBRUSH hGalaxyCore = CreateSolidBrush(RGB(255, 245, 220));
            HBRUSH hGalaxyArm1 = CreateSolidBrush(RGB(160, 80, 50));  
            HBRUSH hGalaxyArm2 = CreateSolidBrush(RGB(60, 130, 100)); 
            HBRUSH hAlienSaucer = CreateSolidBrush(RGB(100, 100, 120));
            HBRUSH hAlienDome = CreateSolidBrush(RGB(50, 255, 100));

            for (const auto& p : planets) {
                float screen_x = cx + (p.x / p.z) * fov;
                float screen_y = cy + (p.y / p.z) * fov;
                float proj_radius = (p.baseRadius / p.z) * fov;

                if (screen_x > -1000 && screen_x < width + 1000 && screen_y > -1000 && screen_y < height + 1000) {
                    HBRUSH pBrush = hAsteroidBrush;
                    
                    if (p.type == 0) pBrush = hEarthBrush;
                    else if (p.type == 1) pBrush = hMarsBrush;
                    else if (p.type == 2) pBrush = hMoonBrush;
                    else if (p.type == 4) pBrush = hVenusBrush;
                    else if (p.type == 5) pBrush = hJupiterBrush;
                    else if (p.type == 6) pBrush = hSaturnBrush;
                    else if (p.type == 7) pBrush = hSunBrush;
                    else if (p.type == 8) pBrush = hGalaxyArm1;
                    else if (p.type == 9) pBrush = hAlienSaucer;

                    if (p.type == 7) { 
                        SelectObject(memDC, hSunCorona);
                        float glowRadius = proj_radius * 1.4f;
                        Ellipse(memDC, (int)(screen_x - glowRadius), (int)(screen_y - glowRadius),
                            (int)(screen_x + glowRadius), (int)(screen_y + glowRadius));
                    }
                    else if (p.type == 0 || p.type == 4) { 
                        SelectObject(memDC, hWhiteBrush);
                        float auraRadius = proj_radius * 1.1f;
                        Ellipse(memDC, (int)(screen_x - auraRadius), (int)(screen_y - auraRadius),
                            (int)(screen_x + auraRadius), (int)(screen_y + auraRadius));
                    }

                    HBRUSH oldB = (HBRUSH)SelectObject(memDC, pBrush);
                    
                    if (p.type != 9) {
                        Ellipse(memDC, (int)(screen_x - proj_radius), (int)(screen_y - proj_radius),
                            (int)(screen_x + proj_radius), (int)(screen_y + proj_radius));
                    }

                    if (p.type == 0) { 
                        SelectObject(memDC, hEarthLandBrush);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 0.4f), (int)(screen_y - proj_radius * 0.6f),
                            (int)(screen_x + proj_radius * 0.5f), (int)(screen_y + proj_radius * 0.1f));
                    }
                    else if (p.type == 2 || p.type == 3) { 
                        SelectObject(memDC, hCraterBrush);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 0.4f), (int)(screen_y - proj_radius * 0.3f),
                            (int)(screen_x - proj_radius * 0.1f), (int)(screen_y + proj_radius * 0.0f));
                    }
                    else if (p.type == 5) { 
                        SelectObject(memDC, hJupiterStripe1);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 0.95f), (int)(screen_y - proj_radius * 0.4f),
                            (int)(screen_x + proj_radius * 0.95f), (int)(screen_y - proj_radius * 0.1f));
                    }
                    else if (p.type == 6) { 
                        SelectObject(memDC, GetStockObject(HOLLOW_BRUSH));
                        SelectObject(memDC, hSaturnRingPen);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 1.8f), (int)(screen_y - proj_radius * 0.6f),
                            (int)(screen_x + proj_radius * 1.8f), (int)(screen_y + proj_radius * 0.3f));
                        SelectObject(memDC, hNullPen);
                    }
                    else if (p.type == 8) { 
                        SelectObject(memDC, hGalaxyArm2);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 1.5f), (int)(screen_y - proj_radius * 0.6f),
                                       (int)(screen_x + proj_radius * 1.5f), (int)(screen_y + proj_radius * 0.6f));
                        SelectObject(memDC, hGalaxyCore);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 0.35f), (int)(screen_y - proj_radius * 0.35f),
                                       (int)(screen_x + proj_radius * 0.35f), (int)(screen_y + proj_radius * 0.35f));
                    }
                    else if (p.type == 9) { 
                        SelectObject(memDC, hAlienDome);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 0.5f), (int)(screen_y - proj_radius * 0.8f),
                                       (int)(screen_x + proj_radius * 0.5f), (int)(screen_y + proj_radius * 0.1f));
                        SelectObject(memDC, hAlienSaucer);
                        Ellipse(memDC, (int)(screen_x - proj_radius * 1.3f), (int)(screen_y - proj_radius * 0.2f),
                                       (int)(screen_x + proj_radius * 1.3f), (int)(screen_y + proj_radius * 0.4f));
                    }

                    if (p.type != 7 && p.type != 8 && p.type != 9) { 
                        SelectObject(memDC, hShadowBrush);
                        float shadow_shift = (screen_x - cx) * 0.12f;
                        Ellipse(memDC, (int)(screen_x - proj_radius * 0.3f + shadow_shift), (int)(screen_y - proj_radius),
                                       (int)(screen_x + proj_radius * 1.0f + shadow_shift), (int)(screen_y + proj_radius));
                    }

                    SelectObject(memDC, oldB);
                }
            }

            // Render Explosions 🔥
            HPEN hExplosionPen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
            SelectObject(memDC, hExplosionPen);
            for (const auto& exp : explosions) {
                float screen_x = cx + (exp.x / exp.z) * fov;
                float screen_y = cy + (exp.y / exp.z) * fov;
                float proj_radius = (exp.baseRadius / exp.z) * fov;
                
                float lifeRatio = (float)exp.timer / (float)exp.maxLife; 
                float currentRadius = proj_radius * (1.8f - lifeRatio); 
                
                if (screen_x > -1000 && screen_x < width + 1000 && screen_y > -1000 && screen_y < height + 1000) {
                    HBRUSH hEdge = CreateSolidBrush(RGB(255, 50, 0));   
                    HBRUSH hMid  = CreateSolidBrush(RGB(255, 140, 0));  
                    HBRUSH hCore = CreateSolidBrush(RGB(255, 255, 50)); 
                    
                    HBRUSH oldB = (HBRUSH)SelectObject(memDC, hEdge);
                    Ellipse(memDC, (int)(screen_x - currentRadius), (int)(screen_y - currentRadius), (int)(screen_x + currentRadius), (int)(screen_y + currentRadius));
                    SelectObject(memDC, hMid);
                    Ellipse(memDC, (int)(screen_x - currentRadius * 0.7f), (int)(screen_y - currentRadius * 0.7f), (int)(screen_x + currentRadius * 0.7f), (int)(screen_y + currentRadius * 0.7f));
                    SelectObject(memDC, hCore);
                    Ellipse(memDC, (int)(screen_x - currentRadius * 0.35f), (int)(screen_y - currentRadius * 0.35f), (int)(screen_x + currentRadius * 0.35f), (int)(screen_y + currentRadius * 0.35f));
                    
                    SelectObject(memDC, oldB);
                    DeleteObject(hEdge); DeleteObject(hMid); DeleteObject(hCore);
                }
            }
            DeleteObject(hExplosionPen);

            // Render Fired Lasers
            HPEN hLaserPen = CreatePen(PS_SOLID, 3, RGB(255, 30, 30));
            SelectObject(memDC, hLaserPen);
            for (auto& l : lasers) {
                if (!l.active) continue;
                float lx1 = cx + (l.x / l.z) * fov;
                float ly1 = cy + (l.y / l.z) * fov;
                float lx2 = cx + (l.x / (l.z - 20.0f)) * fov; 
                float ly2 = cy + (l.y / (l.z - 20.0f)) * fov;
                MoveToEx(memDC, (int)lx1, (int)ly1, NULL);
                LineTo(memDC, (int)lx2, (int)ly2);
            }
            DeleteObject(hLaserPen);

            // Render Neon Crosshair
            HPEN hCrosshairPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); 
            HBRUSH oldHollowBr = (HBRUSH)SelectObject(memDC, GetStockObject(HOLLOW_BRUSH));
            SelectObject(memDC, hCrosshairPen);

            int chX = (int)cx;
            int chY = (int)cy;

            Ellipse(memDC, chX - 15, chY - 15, chX + 15, chY + 15);
            MoveToEx(memDC, chX - 25, chY, NULL); LineTo(memDC, chX - 5, chY);
            MoveToEx(memDC, chX + 5, chY, NULL); LineTo(memDC, chX + 25, chY);
            MoveToEx(memDC, chX, chY - 25, NULL); LineTo(memDC, chX, chY - 5);
            MoveToEx(memDC, chX, chY + 5, NULL); LineTo(memDC, chX, chY + 25);

            SelectObject(memDC, oldHollowBr);
            DeleteObject(hCrosshairPen);

            SetBkMode(memDC, TRANSPARENT);
            HFONT hOldFont = (HFONT)SelectObject(memDC, hCustomFont);
            
            std::wstring hudText = L"SCORE: " + std::to_wstring(g_score) + L"   HULL: " + std::to_wstring(g_hullIntegrity) + L"%";
            SetTextColor(memDC, g_hullIntegrity > 30 ? RGB(0, 255, 0) : RGB(255, 50, 50));
            RECT hudRect = { 15, 45, 400, 70 };
            DrawTextW(memDC, hudText.c_str(), -1, &hudRect, DT_LEFT | DT_SINGLELINE);

            // Draw Virtual Fire Button
            int fx = width - 155;
            int fy = height - 70;
            HBRUSH hFireBase = CreateSolidBrush(RGB(40, 10, 10));
            HBRUSH hFireTop = CreateSolidBrush(g_isPressingFireBtn ? RGB(255, 100, 100) : RGB(200, 0, 0));
            HPEN hFireBorder = CreatePen(PS_SOLID, 2, RGB(255, 50, 50));

            SelectObject(memDC, hFireBase);
            SelectObject(memDC, hFireBorder);
            Ellipse(memDC, fx - 25, fy - 25, fx + 25, fy + 25); 

            int pressOffset = g_isPressingFireBtn ? 2 : 0;
            SelectObject(memDC, hFireTop);
            Ellipse(memDC, fx - 18 + pressOffset, fy - 18 + pressOffset, fx + 18 + pressOffset, fy + 18 + pressOffset); 

            DeleteObject(hFireBase); DeleteObject(hFireTop); DeleteObject(hFireBorder);

            // Draw Virtual Flight Joystick
            int jx = width - 95;
            int jy = height - 70;
            HBRUSH hJoystickBase = CreateSolidBrush(RGB(30, 30, 45));
            HBRUSH hJoystickStick = CreateSolidBrush(RGB(0, 191, 255));
            HPEN hJoystickBorder = CreatePen(PS_SOLID, 2, RGB(0, 191, 255));

            SelectObject(memDC, hJoystickBase);
            SelectObject(memDC, hJoystickBorder);
            Ellipse(memDC, jx - 40, jy - 40, jx + 40, jy + 40); 

            int stickX = jx + (int)(g_joystickOffsetX * 30.0f);
            int stickY = jy + (int)(g_joystickOffsetY * 30.0f);
            SelectObject(memDC, hJoystickStick);
            Ellipse(memDC, stickX - 18, stickY - 18, stickX + 18, stickY + 18); 

            DeleteObject(hJoystickBase); DeleteObject(hJoystickStick); DeleteObject(hJoystickBorder);

            // Draw Throttle Lever
            int tx = width - 35;
            int tTop = height - 120;
            int tBottom = height - 20;
            
            HBRUSH hThrottleTrack = CreateSolidBrush(RGB(20, 20, 30));
            HBRUSH hThrottleHandle = CreateSolidBrush(RGB(255, 140, 0));
            HPEN hThrottleBorder = CreatePen(PS_SOLID, 2, RGB(100, 100, 120));

            SelectObject(memDC, hThrottleTrack);
            SelectObject(memDC, hThrottleBorder);
            RoundRect(memDC, tx - 6, tTop, tx + 6, tBottom, 6, 6);

            int handleY = tBottom - (int)(g_throttleValue * (tBottom - tTop));
            SelectObject(memDC, hThrottleHandle);
            Rectangle(memDC, tx - 12, handleY - 8, tx + 12, handleY + 8);

            DeleteObject(hThrottleTrack); DeleteObject(hThrottleHandle); DeleteObject(hThrottleBorder);
            SelectObject(memDC, hOldFont);
            
            // Clean up Arcade brushes
            DeleteObject(hEarthBrush); DeleteObject(hEarthLandBrush); DeleteObject(hMarsBrush);
            DeleteObject(hMoonBrush); DeleteObject(hCraterBrush); DeleteObject(hAsteroidBrush);
            DeleteObject(hVenusBrush); DeleteObject(hJupiterBrush); DeleteObject(hJupiterStripe1);
            DeleteObject(hJupiterStripe2); DeleteObject(hSaturnBrush); DeleteObject(hSunBrush);
            DeleteObject(hSunCorona); DeleteObject(hShadowBrush); DeleteObject(hGalaxyCore);
            DeleteObject(hGalaxyArm1); DeleteObject(hGalaxyArm2); DeleteObject(hAlienSaucer);
            DeleteObject(hAlienDome);
        }

        // Header and Sci-Fi Border (Drawn in both modes)
        RECT headerRect = { 0, 0, width, 36 };
        FillRect(memDC, &headerRect, hHeaderBrush);

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(220, 220, 220));
        HFONT hOldFontTitle = (HFONT)SelectObject(memDC, hCustomFont);
        RECT titleRect = { 15, 8, 140, 30 };
        DrawTextW(memDC, L"BMI Calculator", -1, &titleRect, DT_LEFT | DT_SINGLELINE);
        SelectObject(memDC, hOldFontTitle);

        HPEN hPanelPen = CreatePen(PS_SOLID, 2, RGB(0, 191, 255)); 
        HBRUSH oldPanelBr = (HBRUSH)SelectObject(memDC, GetStockObject(HOLLOW_BRUSH));
        HPEN oldPanelPn = (HPEN)SelectObject(memDC, hPanelPen);

        RoundRect(memDC, 0, 0, width, height, 14, 14);

        SelectObject(memDC, oldPanelBr); SelectObject(memDC, oldPanelPn);
        DeleteObject(hPanelPen); 

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldPen); SelectObject(memDC, oldBitmap);
        DeleteObject(hNullPen); DeleteObject(hSaturnRingPen); DeleteObject(memBitmap); DeleteDC(memDC);
        
        DeleteObject(hWhiteBrush); DeleteObject(hGrayBrush); DeleteObject(hDimBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORDLG: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndControl = (HWND)lParam;

        if (hwndControl == hResultLabel) SetTextColor(hdcStatic, RGB(0, 191, 255));
        else if (hwndControl == hCategoryLabel) SetTextColor(hdcStatic, currentCategoryColor);
        else SetTextColor(hdcStatic, RGB(50, 205, 50));

        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(255, 51, 51));
        SetBkColor(hdcEdit, RGB(17, 17, 17));
        return (INT_PTR)hDarkGrayBrush;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_BUTTON) {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;
            int id = dis->CtlID;
            bool selected = (dis->itemState & ODS_SELECTED) != 0;

            COLORREF bgColor, txtColor;
            std::wstring textObj;
            const wchar_t* txt = L"";

            if (id == ID_CLEAR_BTN) {
                bgColor = selected ? RGB(40, 40, 40) : (g_clearHovered ? RGB(100, 100, 100) : RGB(70, 70, 70));
                txtColor = RGB(220, 220, 220);
                txt = L"Clear";
            }
            else if (id == ID_MODE_BTN) {
                bgColor = selected ? RGB(60, 100, 180) : RGB(40, 80, 150); 
                txtColor = RGB(255, 255, 255);
                wchar_t btnText[32];
                GetWindowTextW(dis->hwndItem, btnText, 32);
                textObj = btnText;
                txt = textObj.c_str();
            }
            else if (id == ID_MIN_BTN) {
                bgColor = selected ? RGB(50, 50, 60) : (g_minHovered ? RGB(45, 45, 60) : RGB(25, 25, 35));
                txtColor = RGB(200, 200, 200);
                txt = L"_";
            }
            else if (id == ID_MAX_BTN) {
                bgColor = selected ? RGB(50, 50, 60) : (g_maxHovered ? RGB(45, 45, 60) : RGB(25, 25, 35));
                txtColor = RGB(200, 200, 200);
                WINDOWPLACEMENT wp = { sizeof(wp) };
                GetWindowPlacement(hwnd, &wp);
                txt = (wp.showCmd == SW_MAXIMIZE) ? L"-" : L"+";
            }
            else if (id == ID_CLOSE_BTN) {
                bgColor = selected ? RGB(180, 30, 30) : (g_closeHovered ? RGB(232, 17, 35) : RGB(25, 25, 35));
                txtColor = RGB(255, 255, 255);
                txt = L"X";
            }
            else return TRUE;

            HBRUSH hbr = CreateSolidBrush(bgColor);
            HBRUSH oldBr = (HBRUSH)SelectObject(hdc, hbr);
            HPEN oldPn = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
            RoundRect(hdc, rc.left + 4, rc.top + 4, rc.right - 4, rc.bottom - 4, 8, 8);
            SelectObject(hdc, oldBr); SelectObject(hdc, oldPn); DeleteObject(hbr);
            
            SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, txtColor);
            DrawTextW(hdc, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        return TRUE;
    }

    case WM_SIZE: {
        if (hMaxBtn) InvalidateRect(hMaxBtn, NULL, TRUE);
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        
        if (id == ID_MODE_BTN) {
            g_gameMode = !g_gameMode; // Toggle mode
            int showState = g_gameMode ? SW_HIDE : SW_SHOW;
            
            // Hide or Show the calculator inputs
            ShowWindow(hMetricCheck, showState);
            ShowWindow(hFeetLabel, showState);
            ShowWindow(hFeetEdit, showState);
            ShowWindow(hInchesLabel, showState);
            ShowWindow(hInchesEdit, showState);
            ShowWindow(hWeightLabel, showState);
            ShowWindow(hWeightEdit, showState);
            ShowWindow(hClearBtn, showState);
            ShowWindow(hResultLabel, showState);
            ShowWindow(hCategoryLabel, showState);
            
            SetWindowTextW(hModeBtn, g_gameMode ? L"Calc Mode" : L"Arcade Mode");
            SetFocus(hwnd); 
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (id == ID_MIN_BTN) {
            ShowWindow(hwnd, SW_MINIMIZE);
            SetFocus(hwnd);
        }
        else if (id == ID_MAX_BTN) {
            WINDOWPLACEMENT wp = { sizeof(wp) };
            GetWindowPlacement(hwnd, &wp);
            if (wp.showCmd == SW_MAXIMIZE) ShowWindow(hwnd, SW_RESTORE);
            else ShowWindow(hwnd, SW_MAXIMIZE);
            InvalidateRect(hMaxBtn, NULL, TRUE);
            SetFocus(hwnd);
        }
        else if (id == ID_CLOSE_BTN) {
            PostQuitMessage(0);
        }
        else if (HIWORD(wParam) == EN_CHANGE && (id == ID_WEIGHT_EDIT || id == ID_FEET_EDIT || id == ID_INCHES_EDIT)) {
            DoCalculate();
        }
        else if (id == ID_CLEAR_BTN) {
            SetWindowTextW(hWeightEdit, L""); SetWindowTextW(hFeetEdit, L""); SetWindowTextW(hInchesEdit, L"");
            SetWindowTextW(hResultLabel, L""); SetWindowTextW(hCategoryLabel, L"");
            currentCategoryColor = RGB(0, 191, 255);
            InvalidateRect(hCategoryLabel, NULL, TRUE);
            SetFocus(hWeightEdit);
        }
        else if (id == ID_METRIC_CHECK) {
            bool metric = SendMessage(hMetricCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            wchar_t wStr[32] = {}, fStr[32] = {}, iStr[32] = {};
            GetWindowTextW(hWeightEdit, wStr, 32); GetWindowTextW(hFeetEdit, fStr, 32); GetWindowTextW(hInchesEdit, iStr, 32);

            if (metric) {
                SetWindowTextW(hWeightLabel, L"Weight (kg):"); SetWindowTextW(hFeetLabel, L"Height (cm):");
                ShowWindow(hInchesLabel, SW_HIDE); ShowWindow(hInchesEdit, SW_HIDE);
                try {
                    if (wcslen(wStr) > 0) {
                        double kg = std::stod(std::wstring(wStr)) / 2.20462;
                        std::wstringstream ws; ws << std::fixed << std::setprecision(1) << kg;
                        SetWindowTextW(hWeightEdit, ws.str().c_str());
                    }
                    if (wcslen(fStr) > 0) {
                        double ft = std::stod(std::wstring(fStr));
                        double in = (wcslen(iStr) > 0) ? std::stod(std::wstring(iStr)) : 0.0;
                        double cm = ((ft * 12.0) + in) * 2.54;
                        std::wstringstream fs; fs << std::fixed << std::setprecision(1) << cm;
                        SetWindowTextW(hFeetEdit, fs.str().c_str());
                    }
                } catch (...) {}
            }
            else {
                SetWindowTextW(hWeightLabel, L"Weight (lbs):"); SetWindowTextW(hFeetLabel, L"Height (Feet):");
                ShowWindow(hInchesLabel, SW_SHOW); ShowWindow(hInchesEdit, SW_SHOW);
                try {
                    if (wcslen(wStr) > 0) {
                        double lbs = std::stod(std::wstring(wStr)) * 2.20462;
                        std::wstringstream ws; ws << std::fixed << std::setprecision(1) << lbs;
                        SetWindowTextW(hWeightEdit, ws.str().c_str());
                    }
                    if (wcslen(fStr) > 0) {
                        double total_inches = std::stod(std::wstring(fStr)) / 2.54;
                        double ft = std::floor(total_inches / 12.0);
                        double in = total_inches - (ft * 12.0);
                        std::wstringstream fs; fs << std::fixed << std::setprecision(0) << ft; SetWindowTextW(hFeetEdit, fs.str().c_str());
                        std::wstringstream is; is << std::fixed << std::setprecision(1) << in; SetWindowTextW(hInchesEdit, is.str().c_str());
                    }
                } catch (...) {}
            }
            DoCalculate(); 
            SetFocus(hwnd); 
        }
        break;
    }

    case WM_CREATE: {
        srand((unsigned)time(0));
        for (int i = 0; i < NUM_STARS; ++i) {
            stars.push_back({ (float)((rand() % 2000) - 1000), (float)((rand() % 2000) - 1000), (float)((rand() % (int)MAX_DEPTH) + 1), (float)((rand() % 20) + 10) });
        }
        for (int i = 0; i < NUM_PLANETS; ++i) {
            planets.push_back({ (float)((rand() % 4000) - 2000), (float)((rand() % 4000) - 2000), (float)((rand() % (int)MAX_DEPTH) + 1), (float)((rand() % 15) + 8), rand() % 10, (float)((rand() % 80) + 30) });
        }
        SetTimer(hwnd, ID_TIMER, 33, NULL);

        // Header Bar Buttons
        hModeBtn = CreateWindowW(L"BUTTON", L"Arcade Mode", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 155, 4, 120, 28, hwnd, (HMENU)ID_MODE_BTN, NULL, NULL);
        hMinBtn = CreateWindowW(L"BUTTON", L"_", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 296, 4, 36, 28, hwnd, (HMENU)ID_MIN_BTN, NULL, NULL);
        hMaxBtn = CreateWindowW(L"BUTTON", L"+", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 332, 4, 36, 28, hwnd, (HMENU)ID_MAX_BTN, NULL, NULL);
        hCloseBtn = CreateWindowW(L"BUTTON", L"X", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 368, 4, 36, 28, hwnd, (HMENU)ID_CLOSE_BTN, NULL, NULL);
        
        // Calculator UI Elements
        hMetricCheck = CreateWindowW(L"BUTTON", L"Use Metric Units", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 100, 75, 220, 22, hwnd, (HMENU)ID_METRIC_CHECK, NULL, NULL);
        SendMessage(hMetricCheck, WM_SETFONT, (WPARAM)hCustomFont, TRUE);

        hFeetLabel = CreateWindowW(L"STATIC", L"Height (Feet):", WS_VISIBLE | WS_CHILD | SS_CENTER, 50, 108, 300, 20, hwnd, NULL, NULL, NULL);
        hFeetEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP, 100, 130, 200, 25, hwnd, (HMENU)ID_FEET_EDIT, NULL, NULL); SendMessage(hFeetEdit, WM_SETFONT, (WPARAM)hCustomFont, TRUE);
        hInchesLabel = CreateWindowW(L"STATIC", L"Height (Inches):", WS_VISIBLE | WS_CHILD | SS_CENTER, 50, 165, 300, 20, hwnd, NULL, NULL, NULL);
        hInchesEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP, 100, 187, 200, 25, hwnd, (HMENU)ID_INCHES_EDIT, NULL, NULL); SendMessage(hInchesEdit, WM_SETFONT, (WPARAM)hCustomFont, TRUE);
        hWeightLabel = CreateWindowW(L"STATIC", L"Weight (lbs):", WS_VISIBLE | WS_CHILD | SS_CENTER, 50, 222, 300, 20, hwnd, NULL, NULL, NULL);
        hWeightEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP, 100, 244, 200, 25, hwnd, (HMENU)ID_WEIGHT_EDIT, NULL, NULL); SendMessage(hWeightEdit, WM_SETFONT, (WPARAM)hCustomFont, TRUE);
        hClearBtn = CreateWindowW(L"BUTTON", L"Clear", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP, 100, 288, 200, 35, hwnd, (HMENU)ID_CLEAR_BTN, NULL, NULL);
        hResultLabel = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 338, 360, 25, hwnd, (HMENU)ID_RESULT_LABEL, NULL, NULL); SendMessage(hResultLabel, WM_SETFONT, (WPARAM)hCustomFont, TRUE);
        hCategoryLabel = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 368, 360, 25, hwnd, (HMENU)ID_CATEGORY_LABEL, NULL, NULL); SendMessage(hCategoryLabel, WM_SETFONT, (WPARAM)hCustomFont, TRUE);

        g_oldEditProc = (WNDPROC)SetWindowLongPtr(hWeightEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
        SetWindowLongPtr(hFeetEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
        SetWindowLongPtr(hInchesEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        DeleteObject(hBlackBrush); DeleteObject(hDarkGrayBrush); DeleteObject(hHeaderBrush); DeleteObject(hCustomFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void DoCalculate() {
    wchar_t weightStr[32] = {}, feetStr[32] = {}, inchesStr[32] = {};
    GetWindowTextW(hWeightEdit, weightStr, 32); GetWindowTextW(hFeetEdit, feetStr, 32); GetWindowTextW(hInchesEdit, inchesStr, 32);
    bool metric = SendMessage(hMetricCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;

    if (wcslen(weightStr) == 0 || wcslen(feetStr) == 0 || (!metric && wcslen(inchesStr) == 0)) {
        SetWindowTextW(hResultLabel, L""); SetWindowTextW(hCategoryLabel, L""); return;
    }

    try {
        double val1 = std::stod(std::wstring(weightStr)); double val2 = std::stod(std::wstring(feetStr)); double val3 = metric ? 0.0 : std::stod(std::wstring(inchesStr)); double bmi = 0.0;
        if (metric) {
            if (val1 <= 0 || val2 <= 0) { SetWindowTextW(hResultLabel, L"Error:"); SetWindowTextW(hCategoryLabel, L"Enter positive numbers."); currentCategoryColor = RGB(255, 51, 51); InvalidateRect(hCategoryLabel, NULL, TRUE); return; }
            bmi = val1 / ((val2 / 100.0) * (val2 / 100.0));
        } else {
            double total_inches = (val2 * 12.0) + val3;
            if (total_inches <= 0 || val1 <= 0) { SetWindowTextW(hResultLabel, L"Error:"); SetWindowTextW(hCategoryLabel, L"Enter positive numbers."); currentCategoryColor = RGB(255, 51, 51); InvalidateRect(hCategoryLabel, NULL, TRUE); return; }
            bmi = (val1 / (total_inches * total_inches)) * 703.0;
        }

        std::wstring category;
        if (bmi < 18.5) { category = L"(Underweight)"; currentCategoryColor = RGB(173, 216, 230); }
        else if (bmi < 24.9) { category = L"(Normal Weight)"; currentCategoryColor = RGB(50, 205, 50); }
        else if (bmi < 27.5) { category = L"(Slightly Overweight)"; currentCategoryColor = RGB(255, 215, 0); }
        else if (bmi < 29.9) { category = L"(Overweight)"; currentCategoryColor = RGB(255, 140, 0); }
        else if (bmi < 34.9) { category = L"(Obese)"; currentCategoryColor = RGB(255, 69, 0); }
        else { category = L"(Extremely Obese)"; currentCategoryColor = RGB(255, 0, 0); }

        std::wstringstream stream; stream << std::fixed << std::setprecision(1) << L"BMI: " << bmi;
        SetWindowTextW(hResultLabel, stream.str().c_str()); SetWindowTextW(hCategoryLabel, category.c_str());
        InvalidateRect(hCategoryLabel, NULL, TRUE);
    } catch (...) {
        SetWindowTextW(hResultLabel, L"Error:"); SetWindowTextW(hCategoryLabel, L"Numbers only."); currentCategoryColor = RGB(255, 51, 51); InvalidateRect(hCategoryLabel, NULL, TRUE);
    }
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    const wchar_t* CLASS_NAME = L"BMICalculatorClass";
    WNDCLASSW wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = hInstance; wc.lpszClassName = CLASS_NAME; wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)); wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(WS_EX_COMPOSITED, CLASS_NAME, L"BMI Calculator", WS_POPUP | WS_THICKFRAME | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 416, 420, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) return 0;

    RECT rc; GetWindowRect(hwnd, &rc);
    int winW = rc.right - rc.left, winH = rc.bottom - rc.top;
    SetWindowPos(hwnd, NULL, (GetSystemMetrics(SM_CXSCREEN) - winW) / 2, (GetSystemMetrics(SM_CYSCREEN) - winH) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(hwnd, nCmdShow);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND hFocus = GetFocus();
            if (hFocus == hFeetEdit) SetFocus(hInchesEdit);
            else if (hFocus == hInchesEdit) SetFocus(hWeightEdit);
            else if (hFocus == hWeightEdit) SetFocus(hFeetEdit);
            continue;
        }
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    return 0;
}