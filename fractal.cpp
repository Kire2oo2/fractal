#include <windows.h>
#include <complex>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

const int WIDTH = 800;
const int HEIGHT = 800;
const int MAX_ITER = 1000;

std::atomic<bool> running(true);

double xMin = -2.0, xMax = 1.0;
double yMin = -1.5, yMax = 1.5;

void drawMandelbrot(HDC hdc) {
    for (int px = 0; px < WIDTH; ++px) {
        for (int py = 0; py < HEIGHT; ++py) {
            double x0 = xMin + (xMax - xMin) * px / WIDTH;
            double y0 = yMin + (yMax - yMin) * py / HEIGHT;

            std::complex<double> c(x0, y0);
            std::complex<double> z(0, 0);
            int iterations = 0;

            while (std::abs(z) <= 2.0 && iterations < MAX_ITER) {
                z = z * z + c;
                ++iterations;
            }

            COLORREF color = iterations == MAX_ITER
                ? RGB(0, 0, 0)
                : RGB(iterations % 256, iterations % 256, iterations % 256);

            SetPixel(hdc, px, py, color);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        drawMandelbrot(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        double centerX = xMin + (xMax - xMin) * mouseX / WIDTH;
        double centerY = yMin + (yMax - yMin) * mouseY / HEIGHT;

        double zoomFactor = 0.2;
        double newWidth = (xMax - xMin) * zoomFactor;
        double newHeight = (yMax - yMin) * zoomFactor;

        xMin = centerX - newWidth / 2;
        xMax = centerX + newWidth / 2;
        yMin = centerY - newHeight / 2;
        yMax = centerY + newHeight / 2;

        InvalidateRect(hwnd, nullptr, TRUE); // Redraw window
        return 0;
    }
    case WM_DESTROY:
        running = false;
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int main() {
    const wchar_t CLASS_NAME[] = L"MandelbrotWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Mandelbrot Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};
    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
