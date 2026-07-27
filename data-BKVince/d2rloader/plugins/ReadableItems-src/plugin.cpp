#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

#include "readable_items_config.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace {
using ruffneckk::readable_items::Config;
using ruffneckk::readable_items::Entry;
using ruffneckk::readable_items::FindEntry;
using ruffneckk::readable_items::PackItemCode;
using ruffneckk::readable_items::ParseConfig;
using ruffneckk::readable_items::ReaderState;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t GetItemsTxtRecordRva = 0x314110;
constexpr std::uintptr_t GetItemDataContextRva = 0x34A0E0;
constexpr std::uintptr_t GetUnitClassIdRva = 0x349860;
constexpr std::uintptr_t EnsureStringCapacityRva = 0x076210;
constexpr std::size_t ItemCodeOffset = 0x080;
constexpr wchar_t ConfigFileName[] = L"ReadableItems.json";

using GetItemsTxtRecordFn = std::uint8_t*(__fastcall*)(
    std::uint8_t, std::int32_t) noexcept;
using GetItemDataContextFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetUnitClassIdFn = std::uint32_t(__fastcall*)(
    void*, const char*, int) noexcept;
using EnsureStringCapacityFn = void(__fastcall*)(void*, std::size_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
GetItemsTxtRecordFn GetItemsTxtRecord{};
GetItemDataContextFn GetItemDataContext{};
GetUnitClassIdFn GetUnitClassId{};
EnsureStringCapacityFn EnsureStringCapacity{};
Config Settings{};
std::string LoadedConfigPath{"not loaded"};
ReaderState Reader;
std::mutex ReaderMutex;
std::atomic<std::uint64_t> OpenCount{};
std::atomic<std::uint64_t> TooltipCount{};

bool PreviousEscape{};
bool PreviousUp{};
bool PreviousDown{};
bool PreviousPageUp{};
bool PreviousPageDown{};
bool PreviousEnter{};
bool PreviousSpace{};
bool PreviousMouseLeft{};
bool ScrollbarDragging{};
float ScrollbarGrabOffset{};
std::uint64_t LastRevealTick{};
std::uint64_t RevealMilliCharacters{};

constexpr std::uint64_t DialogueCharactersPerSecond = 18;
constexpr int HostedFormalFontIndex = 10;

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "readable-items",
    .name = "Readable Items",
    .version = "0.2.1",
    .author = "RuffnecKk",
    .description = "Opens configured item text when the player right-clicks it.",
    .flags = D2RL::PluginFlags::None,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

bool IsReadableMemory(const void* address, std::size_t size) noexcept {
    if (!address || size == 0) return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(address, &memory, sizeof(memory)) != sizeof(memory)
        || memory.State != MEM_COMMIT
        || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) {
        return false;
    }
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto end = start + size;
    const auto regionEnd = reinterpret_cast<std::uintptr_t>(memory.BaseAddress)
        + memory.RegionSize;
    return end >= start && end <= regionEnd;
}

bool LoadConfig() noexcept {
    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(
            std::filesystem::path(Context->modDirectory) / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);

    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) continue;
            const auto root = nlohmann::json::parse(input, nullptr, true, true);
            Settings = ParseConfig(root);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            if (Context) {
                const auto message = std::string(
                    "ReadableItems: invalid configuration: ") + exception.what();
                Context->LogError(message.c_str());
            }
            return false;
        }
    }

    if (Context) Context->LogError(
        "ReadableItems: ReadableItems.json was not found in the active mod or global scope.");
    return false;
}

const Entry* FindEntryForUnit(void* item) noexcept {
    if (!item || !GetItemDataContext || !GetUnitClassId || !GetItemsTxtRecord) {
        return nullptr;
    }
    const auto context = GetItemDataContext(item);
    const auto classId = GetUnitClassId(item, nullptr, 0);
    auto* record = GetItemsTxtRecord(context, static_cast<std::int32_t>(classId));
    if (!record || !IsReadableMemory(record + ItemCodeOffset, sizeof(std::uint32_t))) {
        return nullptr;
    }
    std::uint32_t code{};
    std::memcpy(&code, record + ItemCodeOffset, sizeof(code));
    return FindEntry(Settings, code);
}

std::string_view Trim(std::string_view value) noexcept {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.remove_suffix(1);
    }
    return value;
}

bool KeyPressed(int virtualKey, bool& previous) noexcept {
    const auto down = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    const auto pressed = down && !previous;
    previous = down;
    return pressed;
}

std::vector<std::string> WrapText(
    std::string_view text,
    ImFont* font,
    float fontSize,
    float width
) {
    std::vector<std::string> lines;
    if (!font || text.empty() || width <= 1.0F) return lines;
    const auto scale = fontSize / font->FontSize;
    const char* cursor = text.data();
    const char* const end = text.data() + text.size();

    while (cursor < end) {
        const auto newline = static_cast<const char*>(
            std::memchr(cursor, '\n', static_cast<std::size_t>(end - cursor)));
        const char* const paragraphEnd = newline ? newline : end;
        if (cursor == paragraphEnd) {
            lines.emplace_back();
        }
        while (cursor < paragraphEnd) {
            const char* wrap = font->CalcWordWrapPositionA(
                scale, cursor, paragraphEnd, width);
            if (!wrap || wrap <= cursor) wrap = std::min(cursor + 1, paragraphEnd);
            const char* visibleEnd = wrap;
            while (visibleEnd > cursor
                && (visibleEnd[-1] == ' ' || visibleEnd[-1] == '\t' || visibleEnd[-1] == '\r')) {
                --visibleEnd;
            }
            lines.emplace_back(cursor, visibleEnd);
            cursor = wrap;
            while (cursor < paragraphEnd && (*cursor == ' ' || *cursor == '\t')) ++cursor;
        }
        if (!newline) break;
        cursor = newline + 1;
    }
    if (lines.empty()) lines.emplace_back();
    return lines;
}

std::size_t Utf8CharacterCount(std::string_view text) noexcept {
    std::size_t count{};
    for (const auto character : text) {
        if ((static_cast<unsigned char>(character) & 0xC0U) != 0x80U) ++count;
    }
    return count;
}

std::size_t Utf8PrefixBytes(
    std::string_view text,
    std::size_t characterCount
) noexcept {
    if (characterCount == 0) return 0;
    std::size_t characters{};
    std::size_t bytes{};
    while (bytes < text.size()) {
        if ((static_cast<unsigned char>(text[bytes]) & 0xC0U) != 0x80U) {
            if (characters == characterCount) break;
            ++characters;
        }
        ++bytes;
    }
    return bytes;
}

std::size_t DialogueCharacterCount(
    const std::vector<std::string>& lines
) noexcept {
    std::size_t count{};
    for (std::size_t index{}; index < lines.size(); ++index) {
        count += Utf8CharacterCount(lines[index]);
        if (index + 1 < lines.size()) ++count;
    }
    return count;
}

ImFont* SelectDialogueFont(ImDrawList* draw) noexcept {
    auto* base = draw && draw->_Data ? draw->_Data->Font : nullptr;
    if (!base || !base->ContainerAtlas) return base;
    const auto& fonts = base->ContainerAtlas->Fonts;
    if (fonts.Size > HostedFormalFontIndex && fonts[HostedFormalFontIndex]) {
        return fonts[HostedFormalFontIndex];
    }
    return base;
}

struct PointerState final {
    ImVec2 position{};
    bool valid{};
    bool pressed{};
    bool down{};
    bool released{};
};

PointerState ReadPointerState(
    HWND window,
    float displayWidth,
    float displayHeight
) noexcept {
    PointerState result;
    result.down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    result.pressed = result.down && !PreviousMouseLeft;
    result.released = !result.down && PreviousMouseLeft;
    PreviousMouseLeft = result.down;
    if (!window) return result;

    POINT cursor{};
    RECT client{};
    if (!GetCursorPos(&cursor) || !ScreenToClient(window, &cursor)
        || !GetClientRect(window, &client)) {
        return result;
    }
    const auto clientWidth = client.right - client.left;
    const auto clientHeight = client.bottom - client.top;
    if (clientWidth <= 0 || clientHeight <= 0) return result;

    result.position.x = static_cast<float>(cursor.x) * displayWidth
        / static_cast<float>(clientWidth);
    result.position.y = static_cast<float>(cursor.y) * displayHeight
        / static_cast<float>(clientHeight);
    result.valid = result.position.x >= 0.0F && result.position.y >= 0.0F
        && result.position.x <= displayWidth && result.position.y <= displayHeight;
    return result;
}

bool Contains(
    const ImVec2& point,
    const ImVec2& minimum,
    const ImVec2& maximum
) noexcept {
    return point.x >= minimum.x && point.y >= minimum.y
        && point.x <= maximum.x && point.y <= maximum.y;
}

void DrawDialogueCorner(
    ImDrawList* draw,
    const ImVec2& corner,
    float horizontalDirection,
    float verticalDirection,
    ImU32 color
) {
    constexpr float longArm = 28.0F;
    constexpr float shortArm = 12.0F;
    const ImVec2 horizontalEnd(
        corner.x + horizontalDirection * longArm, corner.y);
    const ImVec2 verticalEnd(
        corner.x, corner.y + verticalDirection * longArm);
    draw->AddLine(corner, horizontalEnd, color, 2.0F);
    draw->AddLine(corner, verticalEnd, color, 2.0F);
    draw->AddLine(
        ImVec2(corner.x + horizontalDirection * shortArm, corner.y),
        ImVec2(corner.x, corner.y + verticalDirection * shortArm), color, 1.5F);
    draw->AddCircleFilled(
        ImVec2(corner.x + horizontalDirection * 5.0F,
            corner.y + verticalDirection * 5.0F),
        2.2F, color);
}

void DrawReader(
    ImDrawList* draw,
    float displayWidth,
    float displayHeight,
    HWND window
) {
    if (!draw || displayWidth < 640.0F || displayHeight < 480.0F) return;
    if (window && GetForegroundWindow() != window) return;

    std::lock_guard lock(ReaderMutex);
    if (!Reader.IsOpen()) return;

    const float horizontalMargin = std::clamp(displayWidth * 0.025F, 16.0F, 48.0F);
    const float panelWidth = std::min(
        displayWidth - horizontalMargin * 2.0F,
        std::clamp(displayWidth * 0.88F, 620.0F, 1200.0F));
    const float top = std::clamp(displayHeight * 0.018F, 12.0F, 28.0F);
    const float panelHeight = std::min(
        displayHeight - top - 40.0F,
        std::clamp(displayHeight * 0.32F, 270.0F, 410.0F));
    const float left = (displayWidth - panelWidth) * 0.5F;
    const float right = left + panelWidth;
    const float bottom = top + panelHeight;
    const float bodySize = std::clamp(displayHeight * 0.034F, 28.0F, 42.0F);
    const float lineHeight = bodySize * 1.22F;
    const float bodyLeft = left + 48.0F;
    const float bodyRight = right - 72.0F;
    const float bodyTop = top + 42.0F;
    const float bodyBottom = bottom - 54.0F;
    const auto visibleLines = std::max<std::size_t>(
        1, static_cast<std::size_t>((bodyBottom - bodyTop) / lineHeight));

    auto* font = SelectDialogueFont(draw);
    if (!font) return;
    auto lines = WrapText(Reader.Text(), font, bodySize, bodyRight - bodyLeft);
    const auto totalCharacters = DialogueCharacterCount(lines);
    Reader.SetRevealCharacterCount(totalCharacters);

    const auto now = GetTickCount64();
    if (LastRevealTick == 0) LastRevealTick = now;
    const auto elapsed = std::min<std::uint64_t>(now - LastRevealTick, 250);
    LastRevealTick = now;
    RevealMilliCharacters += elapsed * DialogueCharactersPerSecond;
    const auto advance = static_cast<std::size_t>(RevealMilliCharacters / 1000);
    RevealMilliCharacters %= 1000;
    Reader.AdvanceReveal(advance);

    if (KeyPressed(VK_ESCAPE, PreviousEscape)) {
        Reader.Close();
        LastRevealTick = 0;
        RevealMilliCharacters = 0;
        ScrollbarDragging = false;
        return;
    }

    const auto enterPressed = KeyPressed(VK_RETURN, PreviousEnter);
    const auto spacePressed = KeyPressed(VK_SPACE, PreviousSpace);
    const auto fastForward = enterPressed || spacePressed;
    if (fastForward && !Reader.RevealComplete()) {
        Reader.RevealAll();
        Reader.ResumeFollowingLatest();
    }

    std::vector<std::size_t> visibleBytes(lines.size());
    auto remaining = Reader.RevealedCharacters();
    std::size_t revealedLineCount = lines.empty() ? 0 : 1;
    std::size_t latestLine{};
    for (std::size_t index{}; index < lines.size(); ++index) {
        if (index > 0) {
            if (remaining == 0) break;
            --remaining;
        }
        const auto lineCharacters = Utf8CharacterCount(lines[index]);
        const auto shown = std::min(remaining, lineCharacters);
        visibleBytes[index] = Utf8PrefixBytes(lines[index], shown);
        latestLine = index;
        revealedLineCount = index + 1;
        if (remaining < lineCharacters) break;
        remaining -= lineCharacters;
    }
    Reader.SetViewport(std::max<std::size_t>(revealedLineCount, 1), visibleLines);
    Reader.FollowLatestLine(latestLine);

    if (KeyPressed(VK_UP, PreviousUp)) Reader.ScrollLines(-1);
    if (KeyPressed(VK_DOWN, PreviousDown)) Reader.ScrollLines(1);
    if (KeyPressed(VK_PRIOR, PreviousPageUp)) {
        Reader.ScrollLines(-static_cast<std::ptrdiff_t>(visibleLines));
    }
    if (KeyPressed(VK_NEXT, PreviousPageDown)) {
        Reader.ScrollLines(static_cast<std::ptrdiff_t>(visibleLines));
    }

    const float buttonSize = 27.0F;
    const float scrollLeft = right - 39.0F;
    const float scrollRight = scrollLeft + buttonSize;
    const ImVec2 upMinimum(scrollLeft, bodyTop - 4.0F);
    const ImVec2 upMaximum(scrollRight, bodyTop - 4.0F + buttonSize);
    const ImVec2 downMinimum(scrollLeft, bodyBottom - buttonSize + 4.0F);
    const ImVec2 downMaximum(scrollRight, bodyBottom + 4.0F);
    const float trackTop = upMaximum.y + 8.0F;
    const float trackBottom = downMinimum.y - 8.0F;

    const auto renderedLines = std::max<std::size_t>(Reader.RenderedLineCount(), 1);
    const auto maximumOffset = renderedLines > visibleLines
        ? renderedLines - visibleLines
        : 0;
    const float trackHeight = std::max(trackBottom - trackTop, 1.0F);
    const float thumbHeight = maximumOffset == 0
        ? trackHeight
        : std::min(
            trackHeight,
            std::max(24.0F, trackHeight
                * static_cast<float>(visibleLines)
                / static_cast<float>(renderedLines)));
    const float thumbTravel = std::max(trackHeight - thumbHeight, 0.0F);
    const auto calculateThumbTop = [&]() noexcept {
        return maximumOffset == 0
            ? trackTop
            : trackTop + thumbTravel * static_cast<float>(Reader.ScrollOffset())
                / static_cast<float>(maximumOffset);
    };
    float thumbTop = calculateThumbTop();

    constexpr const char* closeLabel = "Close";
    const float closeFontSize = bodySize * 0.62F;
    const auto closeTextSize = font->CalcTextSizeA(
        closeFontSize, FLT_MAX, 0.0F, closeLabel);
    const float closeWidth = std::max(112.0F, closeTextSize.x + 38.0F);
    const float closeHeight = std::max(30.0F, closeTextSize.y + 10.0F);
    const ImVec2 closeMinimum(
        (left + right - closeWidth) * 0.5F, bottom - closeHeight - 10.0F);
    const ImVec2 closeMaximum(
        closeMinimum.x + closeWidth, closeMinimum.y + closeHeight);

    const auto pointer = ReadPointerState(window, displayWidth, displayHeight);
    if (pointer.released || !pointer.down) ScrollbarDragging = false;
    if (pointer.valid && pointer.pressed) {
        if (Contains(pointer.position, closeMinimum, closeMaximum)) {
            Reader.Close();
            LastRevealTick = 0;
            RevealMilliCharacters = 0;
            ScrollbarDragging = false;
            return;
        }
        if (Contains(pointer.position, upMinimum, upMaximum)) {
            Reader.ScrollLines(-1);
        } else if (Contains(pointer.position, downMinimum, downMaximum)) {
            Reader.ScrollLines(1);
        } else if (Contains(
                pointer.position,
                ImVec2(scrollLeft - 7.0F, trackTop),
                ImVec2(scrollRight + 7.0F, trackBottom))
            && maximumOffset > 0 && thumbTravel > 0.0F) {
            const auto overThumb = pointer.position.y >= thumbTop
                && pointer.position.y <= thumbTop + thumbHeight;
            ScrollbarGrabOffset = overThumb
                ? pointer.position.y - thumbTop
                : thumbHeight * 0.5F;
            ScrollbarDragging = true;
        } else if (Contains(
                pointer.position,
                ImVec2(bodyLeft, bodyTop),
                ImVec2(bodyRight, bodyBottom))
            && !Reader.RevealComplete()) {
            Reader.RevealAll();
            Reader.ResumeFollowingLatest();
            std::fill(visibleBytes.begin(), visibleBytes.end(), 0);
            for (std::size_t index{}; index < lines.size(); ++index) {
                visibleBytes[index] = lines[index].size();
            }
            revealedLineCount = lines.size();
            latestLine = lines.empty() ? 0 : lines.size() - 1;
            Reader.SetViewport(std::max<std::size_t>(revealedLineCount, 1), visibleLines);
            Reader.FollowLatestLine(latestLine);
        }
    }
    if (ScrollbarDragging && pointer.valid && pointer.down
        && maximumOffset > 0 && thumbTravel > 0.0F) {
        const auto target = std::clamp(
            pointer.position.y - trackTop - ScrollbarGrabOffset,
            0.0F,
            thumbTravel);
        const auto ratio = target / thumbTravel;
        Reader.SetScrollOffset(static_cast<std::size_t>(std::lround(
            ratio * static_cast<float>(maximumOffset))));
        thumbTop = calculateThumbTop();
    }

    constexpr ImU32 shadow = IM_COL32(0, 0, 0, 165);
    constexpr ImU32 background = IM_COL32(3, 3, 3, 224);
    constexpr ImU32 border = IM_COL32(178, 145, 70, 255);
    constexpr ImU32 innerBorder = IM_COL32(104, 82, 42, 255);
    constexpr ImU32 textShadow = IM_COL32(0, 0, 0, 220);
    constexpr ImU32 textColor = IM_COL32(245, 243, 236, 255);
    constexpr ImU32 scrollFill = IM_COL32(193, 158, 76, 255);
    constexpr ImU32 scrollDark = IM_COL32(52, 39, 20, 245);
    constexpr ImU32 closeHover = IM_COL32(77, 57, 27, 248);

    draw->AddRectFilled(
        ImVec2(left + 7.0F, top + 9.0F),
        ImVec2(right + 7.0F, bottom + 9.0F), shadow);
    draw->AddRectFilled(ImVec2(left, top), ImVec2(right, bottom), background);
    draw->AddRect(ImVec2(left, top), ImVec2(right, bottom), border, 0.0F, 0, 2.0F);
    draw->AddRect(
        ImVec2(left + 6.0F, top + 6.0F),
        ImVec2(right - 6.0F, bottom - 6.0F), innerBorder, 0.0F, 0, 1.0F);
    DrawDialogueCorner(draw, ImVec2(left + 2.0F, top + 2.0F), 1.0F, 1.0F, border);
    DrawDialogueCorner(draw, ImVec2(right - 2.0F, top + 2.0F), -1.0F, 1.0F, border);
    DrawDialogueCorner(draw, ImVec2(left + 2.0F, bottom - 2.0F), 1.0F, -1.0F, border);
    DrawDialogueCorner(draw, ImVec2(right - 2.0F, bottom - 2.0F), -1.0F, -1.0F, border);

    const auto closeHovered = pointer.valid
        && Contains(pointer.position, closeMinimum, closeMaximum);
    draw->AddRectFilled(
        closeMinimum, closeMaximum, closeHovered ? closeHover : scrollDark, 1.0F);
    draw->AddRect(closeMinimum, closeMaximum, border, 1.0F, 0, 1.5F);
    draw->AddRect(
        ImVec2(closeMinimum.x + 4.0F, closeMinimum.y + 4.0F),
        ImVec2(closeMaximum.x - 4.0F, closeMaximum.y - 4.0F),
        innerBorder, 0.0F, 0, 1.0F);
    const ImVec2 closeTextPosition(
        closeMinimum.x + (closeWidth - closeTextSize.x) * 0.5F,
        closeMinimum.y + (closeHeight - closeTextSize.y) * 0.5F);
    draw->AddText(
        font, closeFontSize,
        ImVec2(closeTextPosition.x + 1.0F, closeTextPosition.y + 1.0F),
        textShadow, closeLabel);
    draw->AddText(font, closeFontSize, closeTextPosition, textColor, closeLabel);

    draw->AddRectFilled(upMinimum, upMaximum, scrollDark);
    draw->AddRect(upMinimum, upMaximum, border, 0.0F, 0, 1.5F);
    draw->AddTriangleFilled(
        ImVec2((upMinimum.x + upMaximum.x) * 0.5F, upMinimum.y + 6.0F),
        ImVec2(upMinimum.x + 6.0F, upMaximum.y - 7.0F),
        ImVec2(upMaximum.x - 6.0F, upMaximum.y - 7.0F), scrollFill);
    draw->AddRectFilled(downMinimum, downMaximum, scrollDark);
    draw->AddRect(downMinimum, downMaximum, border, 0.0F, 0, 1.5F);
    draw->AddTriangleFilled(
        ImVec2(downMinimum.x + 6.0F, downMinimum.y + 7.0F),
        ImVec2(downMaximum.x - 6.0F, downMinimum.y + 7.0F),
        ImVec2((downMinimum.x + downMaximum.x) * 0.5F, downMaximum.y - 6.0F),
        scrollFill);
    draw->AddRectFilled(
        ImVec2(scrollLeft + 10.0F, trackTop),
        ImVec2(scrollRight - 10.0F, trackBottom), scrollDark);

    draw->AddRectFilled(
        ImVec2(scrollLeft + 5.0F, thumbTop),
        ImVec2(scrollRight - 5.0F, thumbTop + thumbHeight), scrollFill, 1.0F);
    draw->AddRect(
        ImVec2(scrollLeft + 5.0F, thumbTop),
        ImVec2(scrollRight - 5.0F, thumbTop + thumbHeight), border, 1.0F);

    const auto first = Reader.ScrollOffset();
    const auto last = std::min(revealedLineCount, first + visibleLines);
    draw->PushClipRect(
        ImVec2(bodyLeft, bodyTop), ImVec2(bodyRight, bodyBottom), true);
    float y = bodyTop;
    for (std::size_t index = first; index < last; ++index) {
        const auto byteCount = visibleBytes[index];
        if (byteCount > 0) {
            const auto* beginning = lines[index].data();
            const auto* end = beginning + byteCount;
            draw->AddText(
                font, bodySize, ImVec2(bodyLeft + 1.5F, y + 1.5F), textShadow,
                beginning, end);
            draw->AddText(
                font, bodySize, ImVec2(bodyLeft, y), textColor, beginning, end);
        }
        y += lineHeight;
    }
    draw->PopClipRect();
}

void* TransformTooltip(void* result, void* item) noexcept {
    if (!Settings.enabled || !result || !item || !EnsureStringCapacity
        || !IsReadableMemory(result, 24)) {
        return result;
    }
    const auto* entry = FindEntryForUnit(item);
    if (!entry) return result;

    try {
        const auto* object = static_cast<const std::uint8_t*>(result);
        const auto* data = *reinterpret_cast<char* const*>(object);
        const auto length = *reinterpret_cast<const std::size_t*>(object + 8);
        if (length == 0 || length > 32 * 1024 || !IsReadableMemory(data, length + 1)) {
            return result;
        }
        const std::string original(data, length);
        if (original.find(Settings.tooltip) != std::string::npos) return result;

        auto enhanced = original;
        enhanced.append("\n\xEE\x81\xBE" "1");
        enhanced.append(Settings.tooltip);
        EnsureStringCapacity(result, enhanced.size());
        auto* destination = *reinterpret_cast<char**>(result);
        if (!IsReadableMemory(destination, enhanced.size() + 1)) return result;
        std::memcpy(destination, enhanced.c_str(), enhanced.size() + 1);
        const auto enhancedLength = enhanced.size();
        std::memcpy(static_cast<std::uint8_t*>(result) + 8,
            &enhancedLength, sizeof(enhancedLength));
        ++TooltipCount;
    } catch (...) {
        if (Context) Context->LogWarn(
            "ReadableItems: tooltip transformation failed safely.");
    }
    return result;
}

bool OpenEntry(const Entry& entry, const char* source) noexcept {
    std::lock_guard lock(ReaderMutex);
    if (!Reader.Open(entry, 1, 1)) return false;
    LastRevealTick = GetTickCount64();
    RevealMilliCharacters = 0;
    ScrollbarDragging = false;
    ++OpenCount;
    if (Context) {
        std::array<char, 256> message{};
        std::snprintf(message.data(), message.size(),
            "ReadableItems: opened code=%s title=%s source=%s.",
            entry.code.c_str(), entry.title.c_str(), source ? source : "unknown");
        Context->LogInfo(message.data());
    }
    return true;
}

auto ConsoleCommand(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*
) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    const auto arguments = Trim(command->args
        ? std::string_view(command->args, command->argsLength)
        : std::string_view{});

    if (arguments == "preview") {
        const auto* entry = FindEntry(Settings, PackItemCode("tsc"));
        if (!entry && !Settings.items.empty()) entry = &Settings.items.front();
        if (!entry || !OpenEntry(*entry, "console-preview")) {
            return D2RL::ConsoleCommandResult::Failed;
        }
        command->plugin->WriteConsoleMessage("Readable Items preview opened.");
        return D2RL::ConsoleCommandResult::Handled;
    }
    if (arguments == "close") {
        std::lock_guard lock(ReaderMutex);
        Reader.Close();
        LastRevealTick = 0;
        RevealMilliCharacters = 0;
        ScrollbarDragging = false;
        command->plugin->WriteConsoleMessage("Readable Items reader closed.");
        return D2RL::ConsoleCommandResult::Handled;
    }
    if (!arguments.empty() && arguments != "status") {
        command->plugin->WriteConsoleMessage(
            "Usage: readable-items [status|preview|close].");
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }

    std::array<char, 512> message{};
    bool open{};
    {
        std::lock_guard lock(ReaderMutex);
        open = Reader.IsOpen();
    }
    std::snprintf(message.data(), message.size(),
        "ReadableItems 0.2.1 test: enabled=%s; entries=%zu; reader=%s; opens=%llu; tooltips=%llu; config=%s.",
        Settings.enabled ? "true" : "false", Settings.items.size(),
        open ? "open" : "closed",
        static_cast<unsigned long long>(OpenCount.load()),
        static_cast<unsigned long long>(TooltipCount.load()),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message.data());
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

extern "C" __declspec(dllexport) void* __cdecl
ReadableItemsTransformTooltip(void* result, void* item) noexcept {
    return TransformTooltip(result, item);
}

extern "C" __declspec(dllexport) bool __cdecl
ReadableItemsHandleUseItem(void* item) noexcept {
    if (!Settings.enabled) return false;
    const auto* entry = FindEntryForUnit(item);
    return entry && OpenEntry(*entry, "right-click");
}

extern "C" __declspec(dllexport) void __cdecl
ReadableItemsRenderOverlay(
    void* drawList,
    float displayWidth,
    float displayHeight,
    HWND window
) noexcept {
    try {
        DrawReader(
            static_cast<ImDrawList*>(drawList), displayWidth, displayHeight, window);
    } catch (...) {
        if (Context) Context->LogWarn("ReadableItems: overlay rendering failed safely.");
    }
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
    const D2RL::PluginContext* context
) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!Base) return false;
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("ReadableItems: only D2R build 92777 is supported.");
        return false;
    }

    constexpr std::array<std::uint8_t, 8> itemsExpected{
        0x40,0x57,0x48,0x83,0xEC,0x30,0x8B,0xFA};
    constexpr std::array<std::uint8_t, 16> dataContextExpected{
        0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,
        0x1A,0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C};
    constexpr std::array<std::uint8_t, 16> classIdExpected{
        0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,
        0x1D,0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C};
    constexpr std::array<std::uint8_t, 16> capacityExpected{
        0x4C,0x8B,0xDC,0x49,0x89,0x5B,0x08,0x49,
        0x89,0x6B,0x18,0x49,0x89,0x73,0x20,0x49};
    if (!context->CheckExpectedBytes(
            GetItemsTxtRecordRva, itemsExpected.data(),
            static_cast<std::uint32_t>(itemsExpected.size()))
        || !context->CheckExpectedBytes(
            GetItemDataContextRva, dataContextExpected.data(),
            static_cast<std::uint32_t>(dataContextExpected.size()))
        || !context->CheckExpectedBytes(
            GetUnitClassIdRva, classIdExpected.data(),
            static_cast<std::uint32_t>(classIdExpected.size()))
        || !context->CheckExpectedBytes(
            EnsureStringCapacityRva, capacityExpected.data(),
            static_cast<std::uint32_t>(capacityExpected.size()))) {
        context->LogError(
            "ReadableItems: D2R 3.2.92777 item or tooltip signature mismatch; plugin refused.");
        return false;
    }

    GetItemsTxtRecord = At<GetItemsTxtRecordFn>(GetItemsTxtRecordRva);
    GetItemDataContext = At<GetItemDataContextFn>(GetItemDataContextRva);
    GetUnitClassId = At<GetUnitClassIdFn>(GetUnitClassIdRva);
    EnsureStringCapacity = At<EnsureStringCapacityFn>(EnsureStringCapacityRva);
    if (!LoadConfig()) return false;

    auto registration = D2RL::MakeConsoleCommand(
        "readable-items", ConsoleCommand,
        "Control and inspect the Readable Items test reader.");
    registration.usage = "readable-items [status|preview|close]";
    if (!context->RegisterConsoleCommand(registration)) {
        context->LogWarn("ReadableItems: console command could not be registered.");
    }

    const auto message = std::string("ReadableItems 0.2.1 test loaded from ")
        + LoadedConfigPath
        + "; waiting for delegated tooltip, right-click and renderer hosts.";
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    {
        std::lock_guard lock(ReaderMutex);
        Reader.Close();
        LastRevealTick = 0;
        RevealMilliCharacters = 0;
        ScrollbarDragging = false;
    }
    EnsureStringCapacity = nullptr;
    GetUnitClassId = nullptr;
    GetItemDataContext = nullptr;
    GetItemsTxtRecord = nullptr;
    Base = nullptr;
    Context = nullptr;
}
