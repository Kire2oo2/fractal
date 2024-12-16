#include <windows.h>
#include <complex>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>

const int WIDTH = 800;
const int HEIGHT = 800;
const int BASE_ITER = 250; // Base iterations for normal zoom level
const int MAX_ITER = 10000; // Max iterations to prevent runaway values

std::atomic<bool> running(true);
std::atomic<int> currentMaxIter(BASE_ITER);
std::atomic<bool> useColor(true); // Default to color mode

long double xMin = -2.0, xMax = 1.0;
long double yMin = -1.5, yMax = 1.5;

long double initialXMin = -2.0, initialXMax = 1.0;
long double initialYMin = -1.5, initialYMax = 1.5;

HWND hwnd = nullptr;

std::vector<COLORREF> pixelBuffer(WIDTH* HEIGHT); // Off-screen pixel buffer

// Function to calculate the color based on iteration count and max iterations
COLORREF getColor(int iterations, int maxIter) {
    if (iterations == maxIter) {
        return RGB(0, 0, 0);  // Black for points inside the set
    }

    double t = static_cast<double>(iterations) / maxIter;

    if (useColor.load()) {
        // Color mode
        int r = static_cast<int>(9 * (1 - t) * t * t * t * 255);
        int g = static_cast<int>(15 * (1 - t) * (1 - t) * t * t * 255);
        int b = static_cast<int>(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);

        // Dynamically adjust the color intensity based on the iteration count
        int maxColorValue = 255;
        r = (r > maxColorValue) ? maxColorValue : r;
        g = (g > maxColorValue) ? maxColorValue : g;
        b = (b > maxColorValue) ? maxColorValue : b;

        return RGB(r, g, b);
    }
    else {
        // Grayscale mode
        int gray = static_cast<int>(255 * t);
        return RGB(gray, gray, gray);  // Return grayscale color
    }
}

void drawMandelbrot(HDC hdc) {
    const int numThreads = std::thread::hardware_concurrency(); // Get number of available cores
    const int rowsPerThread = HEIGHT / numThreads;

    auto drawRegion = [&](int startRow, int endRow) {
        for (int px = 0; px < WIDTH; ++px) {
            for (int py = startRow; py < endRow; ++py) {
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

                // Use the getColor function to calculate the appropriate color for this point
                COLORREF color = getColor(iterations, dynamicMaxIter);

                pixelBuffer[py * WIDTH + px] = color;
            }
        }
        };

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? HEIGHT : (i + 1) * rowsPerThread;
        threads.emplace_back(drawRegion, startRow, endRow);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Once drawing is complete, blit the buffer to the screen
    HBITMAP hBitmap = CreateBitmap(WIDTH, HEIGHT, 1, 32, pixelBuffer.data());
    HDC hdcMem = CreateCompatibleDC(hdc);
    SelectObject(hdcMem, hBitmap);

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);

    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
}

void handleUserInput() {
    while (running) {
        std::string command;
        std::cout << "Enter command (iterations <number>, reset, toggle, quit): " << "\n";
        std::getline(std::cin, command);

        if (command.find("iterations") != std::string::npos) {
            int newIterations = std::stoi(command.substr(command.find(' ') + 1));

            // Ensure newIterations does not exceed MAX_ITER
            if (newIterations > MAX_ITER) {
                newIterations = MAX_ITER;
            }

            currentMaxIter.store(newIterations, std::memory_order_relaxed);
            std::cout << "Number of iterations set to " << newIterations << "\n";

            // Debug: Output the updated number of iterations
            std::cout << "Updated Iterations: " << currentMaxIter.load() << "\n";

            // Redraw the window
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        else if (command == "reset") {
            xMin = initialXMin;
            xMax = initialXMax;
            yMin = initialYMin;
            yMax = initialYMax;

            std::cout << "View reset to initial coordinates.\n";

            // Redraw the window
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        else if (command == "toggle") {
            useColor.store(!useColor.load());
            std::cout << "Toggled to " << (useColor.load() ? "color" : "grayscale") << " mode.\n";

            // Redraw the window
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

        xMin = centerX - newWidth / 2;
        xMax = centerX + newWidth / 2;
        yMin = centerY - newHeight / 2;
        yMax = centerY + newHeight / 2;

        // Debug: Output the current zoom level and iteration
        std::cout << "Zoomed to: (" << xMin << ", " << yMin << ") to (" << xMax << ", " << yMax << ")\n";
        std::cout << "Current Iterations: " << currentMaxIter.load() << "\n";

        // Increase the iterations after zoom
        int newIterations = currentMaxIter.load() + 250;  // Increase by a fixed amount or modify logic
        if (newIterations > MAX_ITER) {
            newIterations = MAX_ITER;
        }
        currentMaxIter.store(newIterations);

        std::cout << "Updated Iterations after zoom: " << currentMaxIter.load() << "\n";

        // Redraw the window
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
