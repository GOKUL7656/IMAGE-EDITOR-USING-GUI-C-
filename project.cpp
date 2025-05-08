// Main.cpp

#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <gdiplusTypes.h> // Explicitly include types
#pragma comment (lib,"Gdiplus.lib")

#define ID_OPEN_BUTTON 1001
#define ID_PROCESS_BUTTON 1002
#define ID_BRIGHTNESS_UP_BUTTON 1003
#define ID_SHARPEN_BUTTON 1004
#define ID_CROP_BUTTON 1005
#define ID_IMAGE_DISPLAY 1006 // Changed to avoid conflict
#define WM_IMAGE_LOADED (WM_USER + 1)
#define WM_IMAGE_PROCESSED (WM_USER + 2)

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[] = L"Simple Image Viewer with Processing";
WCHAR szWindowClass[] = L"SimpleImageWindowClass";
Gdiplus::Bitmap* g_pImage = nullptr;
Gdiplus::Bitmap* g_pProcessedImage = nullptr;
HWND hImageDisplay;
HWND hProcessButton; // Handle to the Grayscale button
HWND hBrightnessUpButton; // Handle to the Brightness +20 button
HWND hSharpenButton;
HWND hCropButton;

// Forward declarations:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                LoadImage(HWND hWnd);
void                ProcessImage(HWND hWnd);
Gdiplus::Bitmap* ConvertToGrayscale(Gdiplus::Bitmap* pBitmap);
void                AdjustBrightness(HWND hWnd, INT brightness);
Gdiplus::Bitmap* AdjustImageBrightness(Gdiplus::Bitmap* pBitmap, INT brightness);
Gdiplus::Bitmap* SharpenImage(Gdiplus::Bitmap* pBitmap);
Gdiplus::Bitmap* CropImage(Gdiplus::Bitmap* pBitmap, INT x, INT y, INT width, INT height);
void                DisplayImage(HWND hWnd, Gdiplus::Bitmap* pBitmap, HDC hdc, const RECT& rect);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_pImage) delete g_pImage;
    if (g_pProcessedImage) delete g_pProcessedImage;
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = nullptr;

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 630, 390, nullptr, nullptr, hInstance, nullptr); // Increased window width

    if (!hWnd) {
        return FALSE;
    }

    CreateWindowW(L"BUTTON", L"Open Image", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 10, 100, 30, hWnd, (HMENU)ID_OPEN_BUTTON, hInstance, nullptr);

    hProcessButton = CreateWindowW(L"BUTTON", L"Grayscale", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, 10, 100, 30, hWnd, (HMENU)ID_PROCESS_BUTTON, hInstance, nullptr);
    EnableWindow(hProcessButton, FALSE); // Initially disabled

    hBrightnessUpButton = CreateWindowW(L"BUTTON", L"Brightness +20", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        230, 10, 120, 30, hWnd, (HMENU)ID_BRIGHTNESS_UP_BUTTON, hInstance, nullptr);
    EnableWindow(hBrightnessUpButton, FALSE); // Initially disabled

    hSharpenButton = CreateWindowW(L"BUTTON", L"Sharpen", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        360, 10, 100, 30, hWnd, (HMENU)ID_SHARPEN_BUTTON, hInstance, nullptr);
    EnableWindow(hSharpenButton, FALSE); // Initially disabled

    hCropButton = CreateWindowW(L"BUTTON", L"Crop (50,50,100,100)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        470, 10, 150, 30, hWnd, (HMENU)ID_CROP_BUTTON, hInstance, nullptr);
    EnableWindow(hCropButton, FALSE); // Initially disabled

    hImageDisplay = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | WS_BORDER,
        10, 50, 610, 330, hWnd, (HMENU)ID_IMAGE_DISPLAY, hInstance, nullptr); // Adjusted width and height

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_OPEN_BUTTON) {
            LoadImage(hWnd);
        }
        else if (LOWORD(wParam) == ID_PROCESS_BUTTON) {
            ProcessImage(hWnd);
        }
        else if (LOWORD(wParam) == ID_BRIGHTNESS_UP_BUTTON) {
            AdjustBrightness(hWnd, 20);
        }
        else if (LOWORD(wParam) == ID_SHARPEN_BUTTON) {
            if (g_pImage) {
                if (g_pProcessedImage) delete g_pProcessedImage;
                g_pProcessedImage = SharpenImage(g_pImage);
                PostMessage(hWnd, WM_IMAGE_PROCESSED, 0, 0);
            }
        }
        else if (LOWORD(wParam) == ID_CROP_BUTTON) {
            if (g_pImage) {
                if (g_pProcessedImage) delete g_pProcessedImage;
                Gdiplus::Bitmap* cropped = CropImage(g_pImage, 50, 50, 100, 100); // Example crop
                if (cropped) {
                    g_pProcessedImage = cropped;
                    PostMessage(hWnd, WM_IMAGE_PROCESSED, 0, 0);
                }
                else {
                    MessageBoxW(hWnd, L"Cropping failed (out of bounds?).", L"Error", MB_OK | MB_ICONERROR);
                }
            }
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
                 break;
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        if (lpdis->hwndItem == hImageDisplay) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            DisplayImage(hWnd, g_pProcessedImage ? g_pProcessedImage : g_pImage, hdc, rc);
        }
    }
                    break;
    case WM_IMAGE_LOADED:
        EnableWindow(hProcessButton, TRUE);
        EnableWindow(hBrightnessUpButton, TRUE);
        EnableWindow(hSharpenButton, TRUE);
        EnableWindow(hCropButton, TRUE);
        if (g_pProcessedImage) {
            delete g_pProcessedImage;
            g_pProcessedImage = nullptr;
        }
        InvalidateRect(hImageDisplay, NULL, TRUE);
        break;
    case WM_IMAGE_PROCESSED:
        InvalidateRect(hImageDisplay, NULL, TRUE);
        break;
    case WM_DESTROY:
        if (g_pImage) delete g_pImage;
        if (g_pProcessedImage) delete g_pProcessedImage;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void LoadImage(HWND hWnd) {
    OPENFILENAMEW ofn;
    WCHAR szFile[260];

    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Image Files (*.bmp;*.jpg;*.jpeg;*.png)\0*.bmp;*.jpg;*.jpeg;*.png\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        if (g_pImage) delete g_pImage;
        g_pImage = Gdiplus::Bitmap::FromFile(ofn.lpstrFile);
        if (g_pImage->GetLastStatus() != Gdiplus::Ok) {
            MessageBoxW(hWnd, L"Error loading image.", L"Error", MB_OK | MB_ICONERROR);
            delete g_pImage;
            g_pImage = nullptr;
            return;
        }
        PostMessage(hWnd, WM_IMAGE_LOADED, 0, 0);
    }
}

void ProcessImage(HWND hWnd) {
    if (g_pImage) {
        if (g_pProcessedImage) delete g_pProcessedImage;
        g_pProcessedImage = ConvertToGrayscale(g_pImage);
        PostMessage(hWnd, WM_IMAGE_PROCESSED, 0, 0);
    }
}

Gdiplus::Bitmap* ConvertToGrayscale(Gdiplus::Bitmap* pBitmap) {
    if (!pBitmap) return nullptr;

    INT width = pBitmap->GetWidth();
    INT height = pBitmap->GetHeight();
    Gdiplus::Bitmap* pGrayscaleBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);

    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, width, height);
    pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    Gdiplus::BitmapData grayscaleData;
    pGrayscaleBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &grayscaleData);

    UINT* srcPixels = (UINT*)bitmapData.Scan0;
    UINT* destPixels = (UINT*)grayscaleData.Scan0;
    UINT stride = bitmapData.Stride / 4; // 4 bytes per pixel (ARGB)

    for (INT y = 0; y < height; ++y) {
        for (INT x = 0; x < width; ++x) {
            UINT pixel = srcPixels[y * stride + x];
            BYTE a = (pixel >> 24) & 0xFF;
            BYTE r = (pixel >> 16) & 0xFF;
            BYTE g = (pixel >> 8) & 0xFF;
            BYTE b = pixel & 0xFF;

            BYTE gray = static_cast<BYTE>(0.299 * r + 0.587 * g + 0.114 * b);
            destPixels[y * stride + x] = (a << 24) | (gray << 16) | (gray << 8) | gray;
        }
    }

    pBitmap->UnlockBits(&bitmapData);
    pGrayscaleBitmap->UnlockBits(&grayscaleData);

    return pGrayscaleBitmap;
}

void AdjustBrightness(HWND hWnd, INT brightness) {
    if (g_pImage) {
        if (g_pProcessedImage) delete g_pProcessedImage;
        g_pProcessedImage = AdjustImageBrightness(g_pImage, brightness);
        PostMessage(hWnd, WM_IMAGE_PROCESSED, 0, 0);
    }
}

Gdiplus::Bitmap* AdjustImageBrightness(Gdiplus::Bitmap* pBitmap, INT brightness) {
    if (!pBitmap) return nullptr;

    INT width = pBitmap->GetWidth();
    INT height = pBitmap->GetHeight();
    Gdiplus::Bitmap* pBrightnessBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);

    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, width, height);
    pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    Gdiplus::BitmapData brightnessData;
    pBrightnessBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &brightnessData);

    UINT* srcPixels = (UINT*)bitmapData.Scan0;
    UINT* destPixels = (UINT*)brightnessData.Scan0;
    UINT stride = bitmapData.Stride / 4;

    for (INT y = 0; y < height; ++y) {
        for (INT x = 0; x < width; ++x) {
            UINT pixel = srcPixels[y * stride + x];
            BYTE a = (pixel >> 24) & 0xFF;
            INT r = (pixel >> 16) & 0xFF;
            INT g = (pixel >> 8) & 0xFF;
            INT b = pixel & 0xFF;

            r += brightness;
            if (r < 0) r = 0;
            if (r > 255) r = 255;

            g += brightness;
            if (g < 0) g = 0;
            if (g > 255) g = 255;

            b += brightness;
            if (b < 0) b = 0;
            if (b > 255) b = 255;

            destPixels[y * stride + x] = (a << 24) | (static_cast<BYTE>(r) << 16) | (static_cast<BYTE>(g) << 8) | static_cast<BYTE>(b);
        }
    }

    pBitmap->UnlockBits(&bitmapData);
    pBrightnessBitmap->UnlockBits(&brightnessData);

    return pBrightnessBitmap;
}

Gdiplus::Bitmap* SharpenImage(Gdiplus::Bitmap* pBitmap) {
    if (!pBitmap) return nullptr;

    INT width = pBitmap->GetWidth();
    INT height = pBitmap->GetHeight();
    Gdiplus::Bitmap* pSharpenedBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);

    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, width, height);
    pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    Gdiplus::BitmapData sharpenedData;
    pSharpenedBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &sharpenedData);

    UINT* srcPixels = (UINT*)bitmapData.Scan0;
    UINT* destPixels = (UINT*)sharpenedData.Scan0;
    UINT stride = bitmapData.Stride / 4;

    // Sharpening kernel
    int kernel[3][3] = {
        {-1, -1, -1},
        {-1,  9, -1},
        {-1, -1, -1}
    };

    for (INT y = 1; y < height - 1; ++y) {
        for (INT x = 1; x < width - 1; ++x) {
            INT r = 0, g = 0, b = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    UINT pixel = srcPixels[(y + ky) * stride + (x + kx)];
                    BYTE pr = (pixel >> 16) & 0xFF;
                    BYTE pg = (pixel >> 8) & 0xFF;
                    BYTE pb = pixel & 0xFF;

                    r += pr * kernel[ky + 1][kx + 1];
                    g += pg * kernel[ky + 1][kx + 1];
                    b += pb * kernel[ky + 1][kx + 1];
                }
            }

            BYTE a = (srcPixels[y * stride + x] >> 24) & 0xFF;

            // Clamp the values using conditional statements
            if (r < 0) r = 0;
            if (r > 255) r = 255;
            if (g < 0) g = 0;
            if (g > 255) g = 255;
            if (b < 0) b = 0;
            if (b > 255) b = 255;

            destPixels[y * stride + x] = (a << 24) | (static_cast<BYTE>(r) << 16) | (static_cast<BYTE>(g) << 8) | static_cast<BYTE>(b);
        }
    }

    // Handle borders (simplistic, just copy original)
    for (int y = 0; y < height; ++y) {
        destPixels[y * stride + 0] = srcPixels[y * stride + 0];
        destPixels[y * stride + width - 1] = srcPixels[y * stride + width - 1];
    }
    for (int x = 0; x < width; ++x) {
        destPixels[0 * stride + x] = srcPixels[0 * stride + x];
        destPixels[(height - 1) * stride + x] = srcPixels[(height - 1) * stride + x];
    }

    pBitmap->UnlockBits(&bitmapData);
    pSharpenedBitmap->UnlockBits(&sharpenedData);

    return pSharpenedBitmap;
}

Gdiplus::Bitmap* CropImage(Gdiplus::Bitmap* pBitmap, INT x, INT y, INT width, INT height) {
    if (!pBitmap) return nullptr;

    INT originalWidth = pBitmap->GetWidth();
    INT originalHeight = pBitmap->GetHeight();

    // Ensure the cropping rectangle is within bounds
    if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
        x >= originalWidth || y >= originalHeight ||
        x + width > originalWidth || y + height > originalHeight) {
        return nullptr; // Or handle the error as needed
    }

    Gdiplus::Bitmap* pCroppedBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(pCroppedBitmap);
    Gdiplus::RectF sourceRect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    Gdiplus::RectF destRect(0, 0, static_cast<float>(width), static_cast<float>(height));

    graphics.DrawImage(
        pBitmap,
        destRect,             // Destination rectangle
        static_cast<float>(x), // Source X
        static_cast<float>(y), // Source Y
        static_cast<float>(width), // Source Width
        static_cast<float>(height),// Source Height
        Gdiplus::UnitPixel    // Unit for source coordinates
    );

    return pCroppedBitmap;
}

void DisplayImage(HWND hWnd, Gdiplus::Bitmap* pBitmap, HDC hdc, const RECT& rect) {
    if (g_pProcessedImage) {
        Gdiplus::Graphics graphics(hdc);
        graphics.DrawImage(g_pProcessedImage, (Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top,
            (Gdiplus::REAL)(rect.right - rect.left), (Gdiplus::REAL)(rect.bottom - rect.top));
    }
    else if (pBitmap) {
        Gdiplus::Graphics graphics(hdc);
        graphics.DrawImage(pBitmap, (Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top,
            (Gdiplus::REAL)(rect.right - rect.left), (Gdiplus::REAL)(rect.bottom - rect.top));
    }
    else {
        RECT mutableRect = rect;
        TCHAR text[] = L"No Image Loaded";
        DrawTextW(hdc, text, _countof(text) - 1, &mutableRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}