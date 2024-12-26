// Copyright (c) 2024 Wazer999
// Licensed under the MIT License. See LICENSE file in the project root for full license information.

#include <windows.h>
#include <d2d1.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <conio.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "d2d1")

// Structure representing a snowflake
struct Snowflake {
    float x;          // X position of the snowflake
    float y;          // Y position of the snowflake
    float alpha;      // Transparency
    float speed;      // Falling speed
    float fadeSpeed;  // Transparency reduction speed
    float radius;     // Radius of the snowflake
};

// Global variables for Direct2D
ID2D1Factory* pFactory = nullptr;
ID2D1HwndRenderTarget* pRenderTarget = nullptr;
ID2D1SolidColorBrush* pBrush = nullptr;
std::vector<Snowflake> snowflakes;

// Snowflake parameters
float speed_base = 1.0f;
float fadeSpeed_base = 0.002f;
int max_radius = 3;
std::atomic<int> rand_count = 8;
std::atomic<bool> active = false;

// Window message handler
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Function to create a transparent window
HWND CreateTransparentWindow(HINSTANCE hInstance) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OverlayWindow";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayWindow",
        L"",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, nullptr
    );

    // Set window transparency
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);

    return hwnd;
}

// Initialize Direct2D
void InitDirect2D(HWND hwnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create D2D1Factory", L"Error", MB_OK);
        return;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    hr = pFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        &pRenderTarget
    );
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create HwndRenderTarget", L"Error", MB_OK);
        return;
    }

    // Create a brush for drawing snowflakes
    hr = pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White, 1.0f),
        &pBrush
    );
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create SolidColorBrush", L"Error", MB_OK);
        return;
    }
}

// Update snowflakes
void UpdateSnowflakes() {
    if (!active) {
        // Clear the snowflakes when not active
        snowflakes.clear();
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0)); // Clear the screen with transparent color
        pRenderTarget->EndDraw();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Reduce CPU load
        return;
    }

    // Update positions and transparency of snowflakes
    for (auto& snowflake : snowflakes) {
        snowflake.y += snowflake.speed;
        snowflake.alpha -= snowflake.fadeSpeed;

        // Remove snowflakes that have faded out or moved off the screen
        if (snowflake.alpha <= 0.0f || snowflake.y > GetSystemMetrics(SM_CYSCREEN)) {
            snowflake = snowflakes.back();
            snowflakes.pop_back();
        }
    }

    // Add a new snowflake with a probability of 1/rand_count
    if (std::rand() % rand_count == 0) {
        Snowflake snowflake;
        //Random properties for a new snowflake
        snowflake.x = static_cast<float>(std::rand() % GetSystemMetrics(SM_CXSCREEN));
        snowflake.y = -50.0f;
        snowflake.alpha = 1.0f;
        snowflake.speed = speed_base + static_cast<float>(std::rand() % 300) / 100.0f;
        snowflake.fadeSpeed = fadeSpeed_base * (1 + std::rand() % 10);
        snowflake.radius = 1.0f + static_cast<float>(std::rand() % max_radius);
        snowflakes.push_back(snowflake);
    }
}

// Render snowflakes
void Render() {
    if (!active) {
        // Clear the screen when not active
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0)); // Transparent background
        pRenderTarget->EndDraw();
        return;
    }

    pRenderTarget->BeginDraw();
    pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0)); // Transparent background

    for (const auto& snowflake : snowflakes) {
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(
            D2D1::Point2F(snowflake.x, snowflake.y),
            snowflake.radius, // X radius
            snowflake.radius  // Y radius
        );

        pBrush->SetOpacity(snowflake.alpha);
        pRenderTarget->FillEllipse(ellipse, pBrush);
    }

    HRESULT hr = pRenderTarget->EndDraw();
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Rendering failed", L"Error", MB_OK);
    }
}

// Cleanup Direct2D resources
void Cleanup() {
    if (pBrush) pBrush->Release();
    if (pRenderTarget) pRenderTarget->Release();
    if (pFactory) pFactory->Release();
}

// Console client for user interaction
void ConsoleClient() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::wcout.imbue(std::locale(".UTF-8"));

    std::wcout << L"Select language\n1 - EN\n2 - RU" << std::endl;

    wchar_t buff_key = _getwch();
    if (buff_key == '1') {
        // English interface
        MessageBox(nullptr, L"It may work unstably in fullscreen applications.\nBefore launching games in fullscreen resolution, it is recommended to turn off the snowflakes.\nTry turning them on after launching the game, preferably after opening the game window.", L"Информация", MB_OK);
        while (true) {
            system("cls");
            std::wcout << L"1 - Enable/Disable snowflakes\n"
                << L"2 - Configure the number of snowflakes\n"
                << L"3 - Configure the falling speed of snowflakes\n"
                << L"4 - Configure the disappearance speed of snowflakes\n"
                << L"5 - Configure the maximum radius of snowflakes\n" << std::endl;

            buff_key = _getwch();
            if (buff_key == '1') {
                active = !active;
            }
            else if (buff_key == '2') {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Configure the number of snowflakes\n"
                    << L"Enter a number between 1 and 30 (lower number - more snowflakes): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 30) {
                    std::wcout << L"Invalid number range entered" << std::endl;
                    std::wcout << L"Press any key to return to the main menu" << std::endl;
                    _getwch();
                }
                else {
                    rand_count = static_cast<int>(buff_num);
                }
            }
            else if (buff_key == '3') {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Configure the falling speed of snowflakes\n"
                    << L"Enter a number between 1 and 100 (default is 10): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 2 || buff_num > 100) {
                    std::wcout << L"Invalid number range entered" << std::endl;
                    std::wcout << L"Press any key to return to the main menu" << std::endl;
                    _getwch();
                }
                else {
                    speed_base = static_cast<float>(buff_num) / 10.0f;
                }
            }
            else if (buff_key == '4') {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Configure the disappearance speed of snowflakes\n"
                    << L"Enter a number between 1 and 10 (default is 2): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 10) {
                    std::wcout << L"Invalid number range entered" << std::endl;
                    std::wcout << L"Press any key to return to the main menu" << std::endl;
                    _getwch();
                }
                else {
                    fadeSpeed_base = static_cast<float>(buff_num) / 1000.0f;
                }
            }
            else if (buff_key == '5') {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Configure the maximum radius of snowflakes\n"
                    << L"Enter a number between 1 and 10 (default is 3): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 10) {
                    std::wcout << L"Invalid number range entered" << std::endl;
                    std::wcout << L"Press any key to return to the main menu" << std::endl;
                    _getwch();
                }
                else {
                    max_radius = static_cast<int>(buff_num);
                }
            }
        }
    }
    else if (buff_key == '2') {
        // Russian interface
        MessageBox(nullptr, L"Может работать нестабильно в полноэкранных приложениях.\nПеред запуском игр в полноэкранном разрешении, рекомендуется выключить снежинки.\nПопробуйте включить их после запуска игры, желательно открыв окно игры.", L"Информация", MB_OK);
        while (true)
        {
            system("cls");
            std::wcout << L"1 - Включить/Выключить снежинки\n"
                << L"2 - Настроить количество снежинок\n"
                << L"3 - Настроить скорость падения снежинок\n"
                << L"4 - Настроить скорость исчезновения снежинок\n"
                << L"5 - Настроить максимальный радиус снежинок\n";

            buff_key = _getwch();

            if (buff_key == '1')
            {
                active = !active;
            }
            else if (buff_key == '2')
            {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Настроить количество снежинок\n"
                    << L"Введите число от 1 до 30 (меньше число - больше снежинок): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 30)
                {
                    std::wcout << L"Введён неверный диапазон чисел\n"
                        << L"Чтобы вернуться в главное меню, нажмите любую клавишу\n";
                    _getwch();
                }
                else
                {
                    rand_count = static_cast<int>(buff_num);
                }
            }
            else if (buff_key == '3')
            {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Настроить скорость падения снежинок\n"
                    << L"Введите число от 1 до 100 (по умолчанию 10): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 100)
                {
                    std::wcout << L"Введён неверный диапазон чисел\n"
                        << L"Чтобы вернуться в главное меню, нажмите любую клавишу\n";
                    _getwch();
                }
                else
                {
                    speed_base = static_cast<float>(buff_num) / 10;
                }
            }
            else if (buff_key == '4')
            {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Настроить скорость исчезновения снежинок\n"
                    << L"Введите число от 1 до 10 (по умолчанию 2): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 10)
                {
                    std::wcout << L"Введён неверный диапазон чисел\n"
                        << L"Чтобы вернуться в главное меню, нажмите любую клавишу\n";
                    _getwch();
                }
                else
                {
                    fadeSpeed_base = static_cast<float>(buff_num) / 1000;
                }
            }
            else if (buff_key == '5')
            {
                system("cls");
                int buff_num = 8;
                std::wcout << L"Настроить максимальный радиус снежинок\n"
                    << L"Введите число от 1 до 10 (по умолчанию 3): ";
                std::cin >> buff_num;
                system("cls");
                if (buff_num < 1 || buff_num > 10)
                {
                    std::wcout << L"Введён неверный диапазон чисел\n"
                        << L"Чтобы вернуться в главное меню, нажмите любую клавишу\n";
                    _getwch();
                }
                else
                {
                    max_radius = static_cast<int>(buff_num);
                }
            }
        }
    }
}

// Function to run snowflakes rendering
void RenderRun() {
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // Initialize random number generator
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    HWND hwnd = CreateTransparentWindow(hInstance);
    InitDirect2D(hwnd);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            UpdateSnowflakes();
            Render();
        }
    }

    Cleanup();
}

// Entry point
int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::wcout.imbue(std::locale(".UTF-8"));

    SetConsoleTitle(L"Snow Client (Beta)");

    std::thread consoleThread(ConsoleClient);
    std::thread snowThread(RenderRun);

    consoleThread.join();
    snowThread.join();

    return 0;
}
