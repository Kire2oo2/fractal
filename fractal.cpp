#include <windows.h>
#include <complex>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include <string>

const int WIDTH = 800;
const int HEIGHT = 800;
const int MAX_ITER = 1000;

std::atomic<bool> running(true);
std::atomic<int> currentMaxIter(MAX_ITER);

long double xMin = -2.0, xMax = 1.0;
long double yMin = -1.5, yMax = 1.5;

long double initialXMin = -2.0, initialXMax = 1.0;
long double initialYMin = -1.5, initialYMax = 1.5;

HWND hwnd = nullptr; 

void drawMandelbrot(HDC hdc) {
    for (int px = 0; px < WIDTH; ++px) {
        for (int py = 0; py < HEIGHT; ++py) {
            long double x0 = xMin + (xMax - xMin) * px / WIDTH;
            long double y0 = yMin + (yMax - yMin) * py / HEIGHT;

            std::complex<long double> c(x0, y0);
            std::complex<long double> z(0, 0);
            int iterations = 0;

            int dynamicMaxIter = currentMaxIter.load();
            while (std::abs(z) <= 2.0 && iterations < dynamicMaxIter) {
                z = z * z + c;
                ++iterations;
            }

            COLORREF color = iterations == dynamicMaxIter
                ? RGB(0, 0, 0)
                : RGB(iterations % 256, iterations % 256, iterations % 256);

            SetPixel(hdc, px, py, color);
        }
    }
}

void handleUserInput() {
    while (running) {
        std::string command;
        std::cout << "Enter command (iterations <number>, reset, zoom <factor>, quit): ";
        std::getline(std::cin, command);

        if (command.find("iterations") != std::string::npos) {
            int newIterations = std::stoi(command.substr(command.find(' ') + 1));
            currentMaxIter.store(newIterations);
            std::cout << "Number of iterations set to " << newIterations << "\n";

            // Redraw the window after changing the iterations
            InvalidateRect(hwnd, nullptr, TRUE); 
        }
        else if (command == "reset") {
            xMin = initialXMin;
            xMax = initialXMax;
            yMin = initialYMin;
            yMax = initialYMax;

            std::cout << "View reset to initial coordinates.\n";

            // Redraw the window after resetting
            InvalidateRect(hwnd, nullptr, TRUE); 
        }
        else if (command.find("zoom") != std::string::npos) {
            double zoomFactor = std::stod(command.substr(command.find(' ') + 1));

            // Apply zoom factor
            long double newWidth = (xMax - xMin) * zoomFactor;
            long double newHeight = (yMax - yMin) * zoomFactor;

            long double centerX = (xMax + xMin) / 2;
            long double centerY = (yMax + yMin) / 2;

            xMin = centerX - newWidth / 2;
            xMax = centerX + newWidth / 2;
            yMin = centerY - newHeight / 2;
            yMax = centerY + newHeight / 2;

            std::cout << "Zoom factor applied: " << zoomFactor << "\n";
            std::cout << "Current center coordinates: (" << centerX << ", " << centerY << ")\n";

            // Redraw the window after zoom
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        else if (command == "quit") {
            running = false;
            std::cout << "Exiting program...\n";
        }
        else {
            std::cout << "Unknown command\n";
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

        long double centerX = xMin + (xMax - xMin) * mouseX / WIDTH;
        long double centerY = yMin + (yMax - yMin) * mouseY / HEIGHT;

        long double zoomFactor = 0.1;
        long double newWidth = (xMax - xMin) * zoomFactor;
        long double newHeight = (yMax - yMin) * zoomFactor;

  
        long double centerX = (xMax + xMin) / 2;
        long double centerY = (yMax + yMin) / 2;

        xMin = centerX - newWidth / 2;
        xMax = centerX + newWidth / 2;
        yMin = centerY - newHeight / 2;
        yMax = centerY + newHeight / 2;

        std::cout << "Current Center Coordinates: (" << centerX << ", " << centerY << ")\n";
        InvalidateRect(hwnd, nullptr, TRUE); 
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

    hwnd = CreateWindowEx(
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

    
    std::thread inputThread(handleUserInput);

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

    
    inputThread.join();

    return 0;
}
