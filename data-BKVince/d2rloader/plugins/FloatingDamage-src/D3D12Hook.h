#pragma once

#include <Windows.h>

struct ImFont;

namespace D3D12 {

constexpr int kFloatingDamageFontCount = 12;
using ExternalOverlayCallback = void(__cdecl*)(
    void* drawList, float displayWidth, float displayHeight, HWND window) noexcept;

void SetDllModule(HMODULE module) noexcept;
void SetExternalOverlayCallback(ExternalOverlayCallback callback) noexcept;
bool InstallHooks() noexcept;
void RemoveHooks() noexcept;
ImFont* GetFloatingDamageFont(int index) noexcept;
void GetDisplaySize(float& width, float& height) noexcept;

} // namespace D3D12
