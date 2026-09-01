#include "imgui_settings_panel.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace RuffnecKk::MapSense {
namespace {

constexpr auto PanelTitle = "MapSense###RuffnecKkMapSenseSettings";
constexpr auto ExpandedWidth = 520.0F;
constexpr auto ExpandedHeight = 620.0F;
constexpr auto LauncherWidth = 164.0F;
constexpr auto LauncherHeight = 68.0F;

struct PositionRuntimeState {
    ImGuiContext* context{};
    ImVec2 displaySize{};
    bool initialized{};
    bool previousExpanded{};
    bool positionDirty{};
};

auto GetPositionRuntimeState() noexcept -> PositionRuntimeState& {
    static PositionRuntimeState state{};
    return state;
}

class ScopedPanelStyle final {
public:
    ScopedPanelStyle() noexcept {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0F);

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImVec4{0.86F, 0.80F, 0.67F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            ImVec4{0.025F, 0.018F, 0.014F, 0.98F});
        ImGui::PushStyleColor(
            ImGuiCol_Border,
            ImVec4{0.62F, 0.48F, 0.22F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_TitleBg,
            ImVec4{0.055F, 0.030F, 0.020F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_TitleBgActive,
            ImVec4{0.12F, 0.065F, 0.030F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_FrameBg,
            ImVec4{0.12F, 0.065F, 0.038F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_FrameBgHovered,
            ImVec4{0.22F, 0.12F, 0.055F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_FrameBgActive,
            ImVec4{0.30F, 0.17F, 0.07F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4{0.18F, 0.09F, 0.045F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4{0.34F, 0.20F, 0.075F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4{0.48F, 0.30F, 0.095F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_Header,
            ImVec4{0.19F, 0.10F, 0.045F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_HeaderHovered,
            ImVec4{0.31F, 0.19F, 0.07F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_HeaderActive,
            ImVec4{0.43F, 0.28F, 0.10F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_CheckMark,
            ImVec4{0.86F, 0.67F, 0.24F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_SliderGrab,
            ImVec4{0.62F, 0.48F, 0.22F, 1.0F});
        ImGui::PushStyleColor(
            ImGuiCol_SliderGrabActive,
            ImVec4{0.86F, 0.67F, 0.24F, 1.0F});
    }

    ~ScopedPanelStyle() noexcept {
        ImGui::PopStyleColor(17);
        ImGui::PopStyleVar(3);
    }

    ScopedPanelStyle(const ScopedPanelStyle&) = delete;
    auto operator=(const ScopedPanelStyle&) -> ScopedPanelStyle& = delete;
};

void Invoke(
        const ImGuiSettingsActionCallback callback,
        const ImGuiSettingsAction action) noexcept {
    if (callback != nullptr) {
        callback(action);
    }
}

void UpdateRememberedPosition(
        Config& config,
        const ImVec2 position,
        const ImVec2 size,
        const ImVec2 displaySize) noexcept {
    if (!config.menu.rememberPosition) {
        return;
    }

    const auto rangeX = std::max(0.0F, displaySize.x - size.x);
    const auto rangeY = std::max(0.0F, displaySize.y - size.y);
    config.menu.positionX = rangeX > 0.0F
        ? std::clamp(position.x / rangeX, 0.0F, 1.0F)
        : 0.0F;
    config.menu.positionY = rangeY > 0.0F
        ? std::clamp(position.y / rangeY, 0.0F, 1.0F)
        : 0.0F;
}

void DrawActionButton(
        const char* label,
        const ImGuiSettingsAction action,
        const ImGuiSettingsActionCallback callback) noexcept {
    if (ImGui::Button(label, ImVec2{ImGui::GetContentRegionAvail().x, 0.0F})) {
        Invoke(callback, action);
    }
}

[[nodiscard]] auto MonsterMarkerShapeLabel(
        MonsterMarkerShape shape) noexcept -> const char* {
    switch (shape) {
        case MonsterMarkerShape::X:
            return "X";
        case MonsterMarkerShape::PlayerCross:
            return "Player Cross";
        case MonsterMarkerShape::Dot:
            return "Dot";
    }
    return "Player Cross";
}

[[nodiscard]] auto DrawMonsterMarkerShape(
        MonsterMarkerShape& shape) noexcept -> bool {
    constexpr std::array Shapes{
        MonsterMarkerShape::X,
        MonsterMarkerShape::PlayerCross,
        MonsterMarkerShape::Dot,
    };

    auto changed = false;
    if (ImGui::BeginCombo("Shape", MonsterMarkerShapeLabel(shape))) {
        for (const auto candidate : Shapes) {
            const auto selected = candidate == shape;
            if (ImGui::Selectable(
                    MonsterMarkerShapeLabel(candidate),
                    selected)) {
                shape = candidate;
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

[[nodiscard]] auto ImmunityDisplayStyleLabel(
        ImmunityDisplayStyle style) noexcept -> const char* {
    switch (style) {
        case ImmunityDisplayStyle::ColoredI:
            return "Colored i";
        case ImmunityDisplayStyle::SplitHalo:
            return "Split Halo";
    }
    return "Colored i";
}

[[nodiscard]] auto DrawImmunityDisplayStyle(
        ImmunityDisplayStyle& style) noexcept -> bool {
    constexpr std::array Styles{
        ImmunityDisplayStyle::ColoredI,
        ImmunityDisplayStyle::SplitHalo,
    };

    auto changed = false;
    if (ImGui::BeginCombo("Style", ImmunityDisplayStyleLabel(style))) {
        for (const auto candidate : Styles) {
            const auto selected = candidate == style;
            if (ImGui::Selectable(
                    ImmunityDisplayStyleLabel(candidate),
                    selected)) {
                style = candidate;
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

[[nodiscard]] auto ColorChannelByte(float value) noexcept -> int {
    return static_cast<int>(std::lround(
        std::clamp(value, 0.0F, 1.0F) * 255.0F));
}

void DrawColorDetailsTooltip(
        const RgbaColor& color,
        float dpiScale) noexcept {
    const auto hex = ColorToHex(color);
    char bytes[64]{};
    char normalized[96]{};
    (void)std::snprintf(
        bytes,
        sizeof(bytes),
        "R: %d, G: %d, B: %d, A: %d",
        ColorChannelByte(color.red),
        ColorChannelByte(color.green),
        ColorChannelByte(color.blue),
        ColorChannelByte(color.alpha));
    (void)std::snprintf(
        normalized,
        sizeof(normalized),
        "(%.3f, %.3f, %.3f, %.3f)",
        color.red,
        color.green,
        color.blue,
        color.alpha);

    const auto& style = ImGui::GetStyle();
    const auto lineHeight = ImGui::GetTextLineHeight();
    const auto widestLine = std::max({
        ImGui::CalcTextSize(hex.c_str()).x,
        ImGui::CalcTextSize(bytes).x,
        ImGui::CalcTextSize(normalized).x,
    });
    const ImVec2 tooltipSize{
        widestLine + style.WindowPadding.x * 2.0F,
        lineHeight * 3.0F
            + style.ItemSpacing.y * 2.0F
            + style.WindowPadding.y * 2.0F,
    };

    const auto* viewport = ImGui::GetMainViewport();
    const auto workMinimum = viewport->WorkPos;
    const ImVec2 workMaximum{
        viewport->WorkPos.x + viewport->WorkSize.x,
        viewport->WorkPos.y + viewport->WorkSize.y,
    };
    const auto mouse = ImGui::GetMousePos();
    const auto gap = 10.0F * dpiScale;
    auto tooltipX = mouse.x + gap;
    if (tooltipX + tooltipSize.x > workMaximum.x) {
        tooltipX = mouse.x - tooltipSize.x - gap;
    }
    tooltipX = std::clamp(
        tooltipX,
        workMinimum.x,
        std::max(workMinimum.x, workMaximum.x - tooltipSize.x));

    auto tooltipY = mouse.y - tooltipSize.y - gap;
    if (tooltipY < workMinimum.y) tooltipY = mouse.y + gap;
    tooltipY = std::clamp(
        tooltipY,
        workMinimum.y,
        std::max(workMinimum.y, workMaximum.y - tooltipSize.y));

    ImGui::SetNextWindowPos(
        ImVec2{tooltipX, tooltipY},
        ImGuiCond_Always);
    if (ImGui::BeginTooltip()) {
        ImGui::TextUnformatted(hex.c_str());
        ImGui::TextUnformatted(bytes);
        ImGui::TextUnformatted(normalized);
        ImGui::EndTooltip();
    }
}

[[nodiscard]] auto DrawMonsterMarkerColor(
        RgbaColor& color,
        float dpiScale,
        const char* label = "Color") noexcept -> bool {
    auto channels = std::array<float, 4>{
        color.red,
        color.green,
        color.blue,
        color.alpha,
    };
    const ImVec4 preview{
        channels[0],
        channels[1],
        channels[2],
        channels[3],
    };
    constexpr auto PreviewFlags = ImGuiColorEditFlags_AlphaPreviewHalf
        | ImGuiColorEditFlags_NoTooltip
        | ImGuiColorEditFlags_NoDragDrop;
    const auto previewSize = ImVec2{42.0F * dpiScale, 0.0F};
    const auto popupName = "Color Picker";

    if (ImGui::ColorButton("##Color", preview, PreviewFlags, previewSize)) {
        ImGui::OpenPopup(popupName);
    }
    const auto previewHovered = ImGui::IsItemHovered(
        ImGuiHoveredFlags_Stationary
            | ImGuiHoveredFlags_DelayShort
            | ImGuiHoveredFlags_NoSharedDelay);
    const auto previewActive = ImGui::IsItemActive();
    const auto previewMinimum = ImGui::GetItemRectMin();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);

    if (previewHovered
            && !previewActive
            && !ImGui::IsPopupOpen(popupName)) {
        DrawColorDetailsTooltip(color, dpiScale);
    }

    const auto parentPosition = ImGui::GetWindowPos();
    const auto parentContentMinimum = ImGui::GetWindowContentRegionMin();
    const auto parentContentMaximum = ImGui::GetWindowContentRegionMax();
    const ImVec2 contentMinimum{
        parentPosition.x + parentContentMinimum.x,
        parentPosition.y + parentContentMinimum.y,
    };
    const ImVec2 contentMaximum{
        parentPosition.x + parentContentMaximum.x,
        parentPosition.y + parentContentMaximum.y,
    };
    const auto contentWidth = std::max(
        1.0F,
        contentMaximum.x - contentMinimum.x);
    const auto contentHeight = std::max(
        1.0F,
        contentMaximum.y - contentMinimum.y);
    const ImVec2 popupSize{
        std::min(290.0F * dpiScale, contentWidth),
        std::min(330.0F * dpiScale, contentHeight),
    };
    const ImVec2 popupPosition{
        std::clamp(
            previewMinimum.x,
            contentMinimum.x,
            std::max(contentMinimum.x, contentMaximum.x - popupSize.x)),
        std::clamp(
            previewMinimum.y,
            contentMinimum.y,
            std::max(contentMinimum.y, contentMaximum.y - popupSize.y)),
    };

    ImGui::SetNextWindowPos(popupPosition, ImGuiCond_Always);
    ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);
    auto saveRequested = false;
    if (ImGui::BeginPopup(
            popupName,
            ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoSavedSettings)) {
        constexpr auto PickerFlags = ImGuiColorEditFlags_AlphaBar
            | ImGuiColorEditFlags_NoInputs
            | ImGuiColorEditFlags_NoOptions
            | ImGuiColorEditFlags_NoLabel
            | ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_NoTooltip
            | ImGuiColorEditFlags_PickerHueBar
            | ImGuiColorEditFlags_InputRGB;
        ImGui::SetNextItemWidth(-1.0F);
        (void)ImGui::ColorPicker4("##Picker", channels.data(), PickerFlags);
        color = {channels[0], channels[1], channels[2], channels[3]};
        saveRequested = ImGui::IsItemDeactivatedAfterEdit();
        ImGui::EndPopup();
    }
    return saveRequested;
}

struct ImmunityColorControl final {
    const char* label;
    RgbaColor ImmunityOptions::* color;
};

[[nodiscard]] auto DrawImmunityColors(
        ImmunityOptions& immunities,
        float dpiScale) noexcept -> bool {
    constexpr std::array Controls{
        ImmunityColorControl{"Physical", &ImmunityOptions::physical},
        ImmunityColorControl{"Fire", &ImmunityOptions::fire},
        ImmunityColorControl{"Lightning", &ImmunityOptions::lightning},
        ImmunityColorControl{"Cold", &ImmunityOptions::cold},
        ImmunityColorControl{"Poison", &ImmunityOptions::poison},
        ImmunityColorControl{"Magic", &ImmunityOptions::magic},
    };

    auto saveRequested = false;
    if (ImGui::BeginTable(
            "##ImmunityColors",
            2,
            ImGuiTableFlags_SizingStretchSame)) {
        for (const auto& control : Controls) {
            ImGui::TableNextColumn();
            ImGui::PushID(control.label);
            saveRequested |= DrawMonsterMarkerColor(
                immunities.*(control.color),
                dpiScale,
                control.label);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    return saveRequested;
}

[[nodiscard]] auto DrawMonsterMarkerStyle(
        const char* label,
        MonsterMarkerStyle& style,
        float dpiScale,
        bool drawNameOptions = false) noexcept -> bool {
    ImGui::PushID(label);
    ImGui::SeparatorText(label);

    auto saveRequested = DrawMonsterMarkerShape(style.shape);

    (void)ImGui::SliderFloat(
        "Size",
        &style.size,
        MinimumMonsterMarkerSize,
        MaximumMonsterMarkerSize,
        "%.0f px",
        ImGuiSliderFlags_AlwaysClamp);
    saveRequested |= ImGui::IsItemDeactivatedAfterEdit();

    if (style.shape != MonsterMarkerShape::Dot) {
        (void)ImGui::SliderFloat(
            "Thickness",
            &style.thickness,
            MinimumMonsterMarkerThickness,
            MaximumMonsterMarkerThickness,
            "%.1f px",
            ImGuiSliderFlags_AlwaysClamp);
        saveRequested |= ImGui::IsItemDeactivatedAfterEdit();
    }
    saveRequested |= DrawMonsterMarkerColor(style.color, dpiScale);

    if (drawNameOptions) {
        ImGui::PushID("Names");
        ImGui::SeparatorText("Names");
        saveRequested |= ImGui::Checkbox(
            "Show Names",
            &style.showNames);
        if (style.showNames) {
            (void)ImGui::SliderFloat(
                "Name Size",
                &style.nameSize,
                MinimumAutomapLabelSize,
                MaximumAutomapLabelSize,
                "%.0f px",
                ImGuiSliderFlags_AlwaysClamp);
            saveRequested |= ImGui::IsItemDeactivatedAfterEdit();
            saveRequested |= DrawMonsterMarkerColor(
                style.nameColor,
                dpiScale,
                "Name Color");
        }
        ImGui::PopID();
    }

    ImGui::PopID();
    return saveRequested;
}

[[nodiscard]] auto DrawAutomapLabelOptions(
        const char* sectionLabel,
        const char* enabledLabel,
        AutomapLabelOptions& options,
        float dpiScale,
        const char* description = nullptr) noexcept -> bool {
    ImGui::PushID(sectionLabel);
    ImGui::SeparatorText(sectionLabel);
    auto saveRequested = ImGui::Checkbox(enabledLabel, &options.enabled);
    if (description != nullptr) {
        ImGui::TextDisabled("%s", description);
    }
    if (options.enabled) {
        (void)ImGui::SliderFloat(
            "Text Size",
            &options.size,
            MinimumAutomapLabelSize,
            MaximumAutomapLabelSize,
            "%.0f px",
            ImGuiSliderFlags_AlwaysClamp);
        saveRequested |= ImGui::IsItemDeactivatedAfterEdit();
        saveRequested |= DrawMonsterMarkerColor(
            options.color,
            dpiScale,
            "Text Color");
    }
    ImGui::PopID();
    return saveRequested;
}

[[nodiscard]] auto DrawAutomapObjectOptions(
        const char* sectionLabel,
        const char* enabledLabel,
        AutomapObjectOptions& options,
        float dpiScale) noexcept -> bool {
    ImGui::PushID(sectionLabel);
    ImGui::SeparatorText(sectionLabel);
    auto saveRequested = ImGui::Checkbox(enabledLabel, &options.enabled);
    if (options.enabled) {
        (void)ImGui::SliderFloat(
            "Marker Size",
            &options.size,
            MinimumAutomapObjectSize,
            MaximumAutomapObjectSize,
            "%.0f px",
            ImGuiSliderFlags_AlwaysClamp);
        saveRequested |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::PushID("MarkerColor");
        saveRequested |= DrawMonsterMarkerColor(
            options.color,
            dpiScale,
            "Marker Color");
        ImGui::PopID();
    }
    ImGui::PopID();
    return saveRequested;
}

[[nodiscard]] auto DrawChestOptions(
        ChestOptions& options,
        SuperChestOptions& specialOptions,
        float dpiScale) noexcept -> bool {
    ImGui::PushID("Chests");
    ImGui::SeparatorText("Chests");
    auto saveRequested = ImGui::Checkbox(
        "Show Chests",
        &options.enabled);
    if (options.enabled) {
        (void)ImGui::SliderFloat(
            "Marker Size",
            &options.size,
            MinimumAutomapObjectSize,
            MaximumAutomapObjectSize,
            "%.0f px",
            ImGuiSliderFlags_AlwaysClamp);
        saveRequested |= ImGui::IsItemDeactivatedAfterEdit();

        ImGui::PushID("LockedAccentColor");
        saveRequested |= DrawMonsterMarkerColor(
            options.lockedAccentColor,
            dpiScale,
            "Locked Lock Color");
        ImGui::PopID();
        ImGui::PushID("TrappedAccentColor");
        saveRequested |= DrawMonsterMarkerColor(
            options.trappedAccentColor,
            dpiScale,
            "Trapped Lock Color");
        ImGui::PopID();
        ImGui::TextWrapped(
            "Chest artwork uses the exact PrimeMH image; only the state lock colors are configurable.");
    }

    ImGui::SeparatorText("Special Chests");
    saveRequested |= ImGui::Checkbox(
        "Show Special Chests",
        &specialOptions.enabled);
    if (specialOptions.enabled) {
        ImGui::TextWrapped(
            "PrimeMH's special-chest stars are embedded in the exact artwork.");
    }
    ImGui::PopID();
    return saveRequested;
}

[[nodiscard]] auto DrawNavigationLineOptions(
        const char* sectionLabel,
        const char* enabledLabel,
        NavigationLineOptions& options,
        float dpiScale) noexcept -> bool {
    ImGui::PushID(sectionLabel);
    ImGui::SeparatorText(sectionLabel);
    auto saveRequested = ImGui::Checkbox(enabledLabel, &options.enabled);
    saveRequested |= DrawMonsterMarkerColor(
        options.color,
        dpiScale,
        "Line Color");
    ImGui::PopID();
    return saveRequested;
}

[[nodiscard]] auto DrawCustomLevelLineOptions(
        CustomLevelLineOptions& options,
        float dpiScale) noexcept -> bool {
    ImGui::PushID("CustomLevelLines");
    ImGui::SeparatorText("Custom Levels");
    auto saveRequested = ImGui::Checkbox(
        "Custom Level Lines",
        &options.enabled);
    saveRequested |= DrawMonsterMarkerColor(
        options.color,
        dpiScale,
        "Line Color");
    ImGui::TextDisabled(
        "%zu configured destination%s",
        options.targets.size(),
        options.targets.size() == 1U ? "" : "s");
    ImGui::PopID();
    return saveRequested;
}

} // namespace

auto DrawImGuiSettingsPanel(
        Config& config,
        bool& expanded,
        bool revealMapEnabled,
        const ImGuiSettingsActionCallback actionCallback) noexcept
        -> ImGuiSettingsBounds {
    const ScopedPanelStyle style{};
    const auto& io = ImGui::GetIO();
    const auto dpiScale = std::max(1.0F, io.FontGlobalScale);
    const auto frameExpanded = expanded;
    auto& positionState = GetPositionRuntimeState();
    auto* const currentContext = ImGui::GetCurrentContext();
    if (positionState.context != currentContext) {
        positionState = {};
        positionState.context = currentContext;
    }
    const auto displaySizeChanged = positionState.initialized
        && (positionState.displaySize.x != io.DisplaySize.x
            || positionState.displaySize.y != io.DisplaySize.y);
    const auto sizeModeChanged = positionState.initialized
        && positionState.previousExpanded != frameExpanded;
    const auto reanchorPosition = !positionState.initialized
        || displaySizeChanged
        || sizeModeChanged;
    const auto oldPositionX = config.menu.positionX;
    const auto oldPositionY = config.menu.positionY;
    const auto requestedSize = frameExpanded
        ? ImVec2{
            std::min(ExpandedWidth * dpiScale, io.DisplaySize.x),
            std::min(ExpandedHeight * dpiScale, io.DisplaySize.y)}
        : ImVec2{LauncherWidth * dpiScale, LauncherHeight * dpiScale};

    const auto availableX = std::max(0.0F, io.DisplaySize.x - requestedSize.x);
    const auto availableY = std::max(0.0F, io.DisplaySize.y - requestedSize.y);
    const ImVec2 anchoredPosition{
        std::clamp(config.menu.positionX, 0.0F, 1.0F) * availableX,
        std::clamp(config.menu.positionY, 0.0F, 1.0F) * availableY,
    };
    ImGui::SetNextWindowPos(
        anchoredPosition,
        reanchorPosition ? ImGuiCond_Always : ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(requestedSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.98F);

    auto windowFlags = ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoNavInputs
        | ImGuiWindowFlags_NoNavFocus;
    if (!frameExpanded) {
        windowFlags |= ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse;
    }

    auto windowOpen = true;
    const auto contentVisible = ImGui::Begin(
        PanelTitle,
        frameExpanded ? &windowOpen : nullptr,
        windowFlags);

    const auto windowPosition = ImGui::GetWindowPos();
    const auto windowSize = ImGui::GetWindowSize();
    UpdateRememberedPosition(
        config,
        windowPosition,
        windowSize,
        io.DisplaySize);

    auto saveRequested = false;
    ImGuiSettingsBounds bounds{
        true,
        frameExpanded,
        false,
        windowPosition.x,
        windowPosition.y,
        windowPosition.x + windowSize.x,
        windowPosition.y + windowSize.y,
    };

    if (contentVisible) {
        if (!frameExpanded) {
            if (ImGui::Button(
                    config.featuresEnabled ? "Open" : "Open (OFF)",
                    ImVec2{ImGui::GetContentRegionAvail().x, 0.0F})) {
                expanded = true;
            }
        } else {
            if (ImGui::Button("Collapse")) {
                expanded = false;
            }

            const auto masterLabel = config.featuresEnabled
                ? "Disable MapSense"
                : "Enable MapSense";
            if (ImGui::Button(
                    masterLabel,
                    ImVec2{ImGui::GetContentRegionAvail().x, 0.0F})) {
                config.featuresEnabled = !config.featuresEnabled;
                saveRequested = true;
            }
            if (!config.featuresEnabled) {
                ImGui::TextDisabled(
                    "Features are suspended. Cells already revealed by D2R remain revealed.");
            }

            if (ImGui::CollapsingHeader(
                    "Map & Reveal",
                    ImGuiTreeNodeFlags_DefaultOpen)) {
                (void)ImGui::SliderFloat(
                    "Additions Opacity",
                    &config.overlay.opacity,
                    0.10F,
                    1.0F,
                    "%.2f",
                    ImGuiSliderFlags_AlwaysClamp);
                saveRequested |= ImGui::IsItemDeactivatedAfterEdit();

                ImGui::BeginDisabled(!config.featuresEnabled);
                DrawActionButton(
                    revealMapEnabled
                        ? "Reveal Map: ON (click to disable)"
                        : "Reveal Map: OFF (click to enable)",
                    ImGuiSettingsAction::ToggleRevealMap,
                    actionCallback);
                ImGui::EndDisabled();
            }

            if (ImGui::CollapsingHeader(
                    "Monsters",
                    ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextWrapped(
                    "All monster categories are always visible. Configure "
                    "each category independently. X and Player Cross expose "
                    "Size and Thickness; Dot exposes Size only.");

                saveRequested |= DrawMonsterMarkerStyle(
                    "Normal",
                    config.monsters.normal,
                    dpiScale);
                saveRequested |= DrawMonsterMarkerStyle(
                    "Minion",
                    config.monsters.minion,
                    dpiScale);
                saveRequested |= DrawMonsterMarkerStyle(
                    "Champion",
                    config.monsters.champion,
                    dpiScale);
                saveRequested |= DrawMonsterMarkerStyle(
                    "Unique",
                    config.monsters.unique,
                    dpiScale);
                saveRequested |= DrawMonsterMarkerStyle(
                    "Super Unique / Boss",
                    config.monsters.superUniqueBoss,
                    dpiScale,
                    true);
            }

            if (ImGui::CollapsingHeader("Immunities")) {
                saveRequested |= ImGui::Checkbox(
                    "Show Immunities",
                    &config.immunities.enabled);

                if (config.immunities.enabled) {
                    saveRequested |= DrawImmunityDisplayStyle(
                        config.immunities.style);

                    if (config.immunities.style
                            == ImmunityDisplayStyle::ColoredI) {
                        (void)ImGui::SliderFloat(
                            "Indicator Size",
                            &config.immunities.indicatorSize,
                            MinimumImmunityIndicatorSize,
                            MaximumImmunityIndicatorSize,
                            "%.0f px",
                            ImGuiSliderFlags_AlwaysClamp);
                    } else {
                        (void)ImGui::SliderFloat(
                            "Halo Thickness",
                            &config.immunities.haloThickness,
                            MinimumImmunityHaloThickness,
                            MaximumImmunityHaloThickness,
                            "%.1f px",
                            ImGuiSliderFlags_AlwaysClamp);
                    }
                    saveRequested |= ImGui::IsItemDeactivatedAfterEdit();

                    ImGui::SeparatorText("Colors");
                    saveRequested |= DrawImmunityColors(
                        config.immunities,
                        dpiScale);
                }
            }

            if (ImGui::CollapsingHeader("Objects")) {
                saveRequested |= ImGui::Checkbox(
                    "Show Automap Objects",
                    &config.objects.enabled);

                if (config.objects.enabled) {
                    saveRequested |= DrawAutomapLabelOptions(
                        "Exit Labels",
                        "Show Exit Labels",
                        config.objects.exitLabels,
                        dpiScale);
                    saveRequested |= DrawAutomapLabelOptions(
                        "Waypoint Labels",
                        "Show Waypoint Labels",
                        config.objects.waypointLabels,
                        dpiScale,
                        "Shows the current area's localized name above D2R's native waypoint icon.");
                    saveRequested |= DrawAutomapLabelOptions(
                        "Shrine Labels",
                        "Show Shrine Labels",
                        config.objects.shrineLabels,
                        dpiScale,
                        "Shows the localized buff name near D2R's native shrine marker.");
                    saveRequested |= DrawChestOptions(
                        config.objects.chests,
                        config.objects.superChests,
                        dpiScale);
                    saveRequested |= DrawAutomapObjectOptions(
                        "Armor Racks",
                        "Show Armor Racks",
                        config.objects.armorRacks,
                        dpiScale);
                    saveRequested |= DrawAutomapObjectOptions(
                        "Weapon Racks",
                        "Show Weapon Racks",
                        config.objects.weaponRacks,
                        dpiScale);
                }
            }

            if (ImGui::CollapsingHeader("Navigation")) {
                (void)ImGui::SliderFloat(
                    "Line Thickness",
                    &config.navigation.lineThickness,
                    MinimumNavigationLineThickness,
                    MaximumNavigationLineThickness,
                    "%.1f px",
                    ImGuiSliderFlags_AlwaysClamp);
                saveRequested |= ImGui::IsItemDeactivatedAfterEdit();

                saveRequested |= DrawNavigationLineOptions(
                    "Waypoint",
                    "Waypoint Line",
                    config.navigation.waypoint,
                    dpiScale);
                saveRequested |= DrawNavigationLineOptions(
                    "Main Progression",
                    "Main Progression Line",
                    config.navigation.progression,
                    dpiScale);
                saveRequested |= DrawNavigationLineOptions(
                    "Quest Targets",
                    "Quest Target Line",
                    config.navigation.quests,
                    dpiScale);
                saveRequested |= DrawCustomLevelLineOptions(
                    config.navigation.customLevels,
                    dpiScale);
            }
        }
    }

    if (frameExpanded && !windowOpen) {
        expanded = false;
    }

    const auto positionChanged = oldPositionX != config.menu.positionX
        || oldPositionY != config.menu.positionY;
    if (positionChanged && !reanchorPosition && config.menu.rememberPosition)
        positionState.positionDirty = true;
    if (positionState.positionDirty
        && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        saveRequested = true;
        positionState.positionDirty = false;
    }
    if (!config.menu.rememberPosition)
        positionState.positionDirty = false;
    bounds.saveRequested = saveRequested;

    positionState.displaySize = io.DisplaySize;
    positionState.previousExpanded = frameExpanded;
    positionState.initialized = true;

    ImGui::End();
    return bounds;
}

} // namespace RuffnecKk::MapSense
