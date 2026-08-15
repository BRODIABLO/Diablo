#Requires AutoHotkey v2.0
#SingleInstance Off
#UseHook

SendMode("Event")
SetTitleMatchMode(3)
CoordMode("Mouse", "Screen")
CoordMode("Pixel", "Screen")

global gInstanceMutex := DllCall("kernel32\CreateMutex", "Ptr", 0, "Int", false,
    "Str", "Local\RuffnecKk.BKVince.GameTestRunner.AHK", "Ptr")
if !gInstanceMutex {
    FileAppend("GameTestRunner could not create its input mutex.`n", "*", "UTF-8-RAW")
    ExitApp(1)
}
if A_LastError = 183 {
    DllCall("kernel32\CloseHandle", "Ptr", gInstanceMutex)
    gInstanceMutex := 0
    FileAppend("Another GameTestRunner AHK instance already owns the input mutex.`n", "*", "UTF-8-RAW")
    ExitApp(1)
}
OnExit(ReleaseInstanceMutex)

global gPlanPath := ""
global gPlanDirectory := ""
global gRunner := Map()
global gProfile := Map()
global gProbes := Map()
global gSteps := []
global gCurrentStep := 0
global gCurrentAction := "runner"
global gInitialized := false
global gRecentInputs := []

Hotkey("Pause", EmergencyStop)

exitCode := 1
try {
    exitCode := Main()
} catch as err {
    message := "Runner initialization failed: " . err.Message
    ConsoleWrite(message)
    if gInitialized {
        try LogEvent(gCurrentStep, gCurrentAction, "failed", "FAILED", 0, message)
    }
    exitCode := 1
}
ExitApp(exitCode)


class RunnerFailure extends Error {
    __New(message, status := "failed") {
        super.__New(message)
        this.Status := status
    }
}


Main() {
    global gPlanPath, gPlanDirectory, gRunner, gSteps
    global gCurrentStep, gCurrentAction, gInitialized

    if A_Args.Length != 1 {
        throw Error("Usage: GameTestRunner.ahk <plan.ini>")
    }

    gPlanPath := A_Args[1]
    if !FileExist(gPlanPath) {
        throw Error("Plan INI does not exist: " . gPlanPath)
    }

    SplitPath(gPlanPath, , &gPlanDirectory)
    LoadPlan()

    DirCreate(gRunner["RunDirectory"])
    eventDirectory := ""
    SplitPath(gRunner["EventLogPath"], , &eventDirectory)
    if eventDirectory != "" {
        DirCreate(eventDirectory)
    }
    eventFile := FileOpen(gRunner["EventLogPath"], "w", "UTF-8-RAW")
    if !IsObject(eventFile) {
        throw Error("Cannot create event log: " . gRunner["EventLogPath"])
    }
    eventFile.Close()
    gInitialized := true

    for index, step in gSteps {
        gCurrentStep := index
        gCurrentAction := step["Action"]
        started := A_TickCount
        artifactPath := ""

        try {
            outcome := ExecuteStep(index, step)
            duration := TickDuration(started)
            state := SafeGetState()
            if outcome.Has("ArtifactPath") {
                artifactPath := outcome["ArtifactPath"]
            }
            LogEvent(index, step["Action"], "completed", state, duration,
                outcome["Message"], artifactPath)
        } catch as err {
            duration := TickDuration(started)
            status := HasProp(err, "Status") ? err.Status : "failed"
            if status != "inconclusive" {
                status := "failed"
            }

            failurePath := TryFailureCapture(index, step["Action"])
            message := err.Message . RecentInputSuffix()
            LogEvent(index, step["Action"], status, SafeGetState(), duration,
                message, failurePath)
            ConsoleWrite("Step " . index . " (" . step["Action"] . ") " . status . ": " . message)
            TryRecovery(index)
            return (status = "inconclusive") ? 2 : 1
        }
    }

    gCurrentStep := 0
    gCurrentAction := "runner"
    return 0
}


LoadPlan() {
    global gRunner, gProfile, gProbes, gSteps

    gRunner := Map()
    gRunner["ScenarioId"] := ReadRequired("Runner", "ScenarioId")
    gRunner["RunDirectory"] := ResolvePlanPath(ReadRequired("Runner", "RunDirectory"))
    gRunner["EventLogPath"] := ResolvePlanPath(ReadRequired("Runner", "EventLogPath"))
    gRunner["WindowTitle"] := ReadRequired("Runner", "WindowTitle")
    gRunner["WorkingDirectory"] := RTrim(ReadRequired("Runner", "WorkingDirectory"), "\/")
    if !InStr(FileExist(gRunner["WorkingDirectory"]), "D") {
        throw Error("Runner.WorkingDirectory must be an existing D2R directory")
    }
    if gRunner["WindowTitle"] != "Diablo II: Resurrected" {
        throw Error("Runner.WindowTitle is not the fixed D2R title")
    }
    gRunner["StepCount"] := ReadInteger("Runner", "StepCount", "__GAME_TEST_REQUIRED__", 1, 999)

    processNamesRaw := ReadRequired("Runner", "ProcessNames")
    processNames := []
    for rawName in StrSplit(processNamesRaw, "|") {
        name := StrLower(Trim(rawName))
        if name = "" {
            continue
        }
        if !RegExMatch(name, "^[a-z0-9_.-]+$") {
            throw Error("Unsafe process name in allowlist: " . rawName)
        }
        if SubStr(name, -4) != ".exe" {
            name .= ".exe"
        }
        if name != "d2r.exe" && name != "d2rloader.exe" {
            throw Error("Runner process allowlist is restricted to D2R.exe and D2RLoader.exe")
        }
        processNames.Push(name)
    }
    if processNames.Length = 0 {
        throw Error("Runner.ProcessNames must contain at least one executable name")
    }
    gRunner["ProcessNames"] := processNames

    gProfile := Map()
    gProfile["ExpectedWidth"] := ReadInteger("Profile", "ExpectedWidth", "__GAME_TEST_REQUIRED__", 1, 32768)
    gProfile["ExpectedHeight"] := ReadInteger("Profile", "ExpectedHeight", "__GAME_TEST_REQUIRED__", 1, 32768)
    gProfile["GeometryTolerancePixels"] := ReadInteger("Profile", "GeometryTolerancePixels", 8, 0, 1000)
    gProfile["InventoryKey"] := ValidateSingleKey(ReadText("Profile", "InventoryKey", "i"), "InventoryKey")
    gProfile["EscapeKey"] := ValidateSingleKey(ReadText("Profile", "EscapeKey", "Esc"), "EscapeKey")

    ReadNormalizedPoint("Play", true)
    ReadNormalizedPoint("Difficulty", false)
    ReadNormalizedPoint("SaveAndExit", true)
    ReadNormalizedPoint("CharacterFallback", false)

    gProfile["DifficultyEnabled"] := ReadBoolean("Profile", "DifficultyEnabled", false)
    if gProfile["DifficultyEnabled"] && !gProfile["DifficultyConfigured"] {
        throw Error("DifficultyEnabled requires DifficultyX and DifficultyY")
    }

    characterTemplate := ReadText("Profile", "CharacterTemplatePath", "")
    gProfile["CharacterTemplatePath"] := characterTemplate = "" ? "" : ResolvePlanPath(characterTemplate)
    gProfile["CharacterTemplateVariation"] := ReadInteger("Profile", "CharacterTemplateVariation", 20, 0, 255)
    gProfile["CharacterSearchX1"] := ReadNumber("Profile", "CharacterSearchX1", 0.0, 0.0, 1.0)
    gProfile["CharacterSearchY1"] := ReadNumber("Profile", "CharacterSearchY1", 0.0, 0.0, 1.0)
    gProfile["CharacterSearchX2"] := ReadNumber("Profile", "CharacterSearchX2", 1.0, 0.0, 1.0)
    gProfile["CharacterSearchY2"] := ReadNumber("Profile", "CharacterSearchY2", 1.0, 0.0, 1.0)
    ValidateNormalizedRectangle("CharacterSearch")
    gProfile["CharacterClickOffsetX"] := ReadNumber("Profile", "CharacterClickOffsetX", 0.0, -1.0, 1.0)
    gProfile["CharacterClickOffsetY"] := ReadNumber("Profile", "CharacterClickOffsetY", 0.0, -1.0, 1.0)

    gProfile["WindowTimeoutMs"] := ReadInteger("Profile", "WindowTimeoutMs", 120000, 1, 900000)
    gProfile["CharacterSelectTimeoutMs"] := ReadInteger("Profile", "CharacterSelectTimeoutMs", 45000, 1, 900000)
    gProfile["InGameTimeoutMs"] := ReadInteger("Profile", "InGameTimeoutMs", 90000, 1, 900000)
    gProfile["InventoryTimeoutMs"] := ReadInteger("Profile", "InventoryTimeoutMs", 5000, 1, 120000)
    gProfile["SaveExitTimeoutMs"] := ReadInteger("Profile", "SaveExitTimeoutMs", 30000, 1, 300000)
    gProfile["PollIntervalMs"] := ReadInteger("Profile", "PollIntervalMs", 250, 25, 5000)
    gProfile["FocusSettleMs"] := ReadInteger("Profile", "FocusSettleMs", 350, 0, 30000)
    gProfile["AfterCharacterClickMs"] := ReadInteger("Profile", "AfterCharacterClickMs", 300, 0, 30000)
    gProfile["AfterPlayClickMs"] := ReadInteger("Profile", "AfterPlayClickMs", 1000, 0, 60000)
    gProfile["AfterDifficultyClickMs"] := ReadInteger("Profile", "AfterDifficultyClickMs", 1000, 0, 60000)
    gProfile["AfterInventoryKeyMs"] := ReadInteger("Profile", "AfterInventoryKeyMs", 150, 0, 30000)
    gProfile["AfterEscapeMs"] := ReadInteger("Profile", "AfterEscapeMs", 500, 0, 30000)
    gProfile["AfterSaveExitClickMs"] := ReadInteger("Profile", "AfterSaveExitClickMs", 1000, 0, 60000)
    gProfile["InputDurationMs"] := ReadInteger("Profile", "InputDurationMs", 25, 0, 5000)
    gProfile["MouseMoveSpeed"] := ReadInteger("Profile", "MouseMoveSpeed", 0, 0, 100)

    gProbes := Map()
    for probeName in ["CharacterSelect", "InGame", "EscMenu", "Inventory"] {
        gProbes[probeName] := LoadProbe(probeName)
    }

    gSteps := []
    Loop gRunner["StepCount"] {
        section := "Step." . Format("{:03}", A_Index)
        action := StrLower(Trim(ReadRequired(section, "Action")))
        name := Trim(ReadText(section, "Name", ""))
        gSteps.Push(Map("Action", action, "Name", name))
    }
}


LoadProbe(name) {
    global gProfile

    pathKey := name . "TemplatePath"
    path := ReadText("Profile", pathKey, "")
    probe := Map()
    probe["Path"] := path = "" ? "" : ResolvePlanPath(path)
    probe["X1"] := ReadNumber("Profile", name . "SearchX1", 0.0, 0.0, 1.0)
    probe["Y1"] := ReadNumber("Profile", name . "SearchY1", 0.0, 0.0, 1.0)
    probe["X2"] := ReadNumber("Profile", name . "SearchX2", 1.0, 0.0, 1.0)
    probe["Y2"] := ReadNumber("Profile", name . "SearchY2", 1.0, 0.0, 1.0)
    probe["Variation"] := ReadInteger("Profile", name . "Variation", 20, 0, 255)
    if probe["X1"] > probe["X2"] || probe["Y1"] > probe["Y2"] {
        throw Error(name . " probe search rectangle is inverted")
    }
    return probe
}


ReadNormalizedPoint(prefix, required) {
    global gProfile

    sentinel := "__GAME_TEST_MISSING__"
    rawX := IniRead(gPlanPath, "Profile", prefix . "X", sentinel)
    rawY := IniRead(gPlanPath, "Profile", prefix . "Y", sentinel)
    hasX := rawX != sentinel && Trim(rawX) != ""
    hasY := rawY != sentinel && Trim(rawY) != ""
    configured := hasX && hasY

    if required && !configured {
        throw Error("Profile." . prefix . "X and " . prefix . "Y are required")
    }
    if hasX != hasY {
        throw Error("Profile." . prefix . "X and " . prefix . "Y must be configured together")
    }
    if configured {
        if !IsNumber(rawX) || !IsNumber(rawY) {
            throw Error("Profile." . prefix . " coordinates must be numeric")
        }
        x := rawX + 0
        y := rawY + 0
        if x <= 0 || x >= 1 || y <= 0 || y >= 1 {
            throw Error("Profile." . prefix . " coordinates must be strictly between 0 and 1")
        }
        gProfile[prefix . "X"] := x
        gProfile[prefix . "Y"] := y
    }
    gProfile[prefix . "Configured"] := configured
}


ValidateNormalizedRectangle(prefix) {
    global gProfile
    if gProfile[prefix . "X1"] > gProfile[prefix . "X2"]
        || gProfile[prefix . "Y1"] > gProfile[prefix . "Y2"] {
        throw Error("Profile." . prefix . " rectangle is inverted")
    }
}


ReadRequired(section, key) {
    sentinel := "__GAME_TEST_MISSING__"
    value := IniRead(gPlanPath, section, key, sentinel)
    if value = sentinel || Trim(value) = "" {
        throw Error("Missing required INI value [" . section . "] " . key)
    }
    return Trim(value)
}


ReadText(section, key, defaultValue := "") {
    value := IniRead(gPlanPath, section, key, defaultValue)
    return Trim(value)
}


ReadNumber(section, key, defaultValue := "__GAME_TEST_REQUIRED__", minimum := "", maximum := "") {
    sentinel := "__GAME_TEST_MISSING__"
    raw := IniRead(gPlanPath, section, key, sentinel)
    if raw = sentinel || Trim(raw) = "" {
        if defaultValue = "__GAME_TEST_REQUIRED__" {
            throw Error("Missing numeric INI value [" . section . "] " . key)
        }
        value := defaultValue
    } else {
        if !IsNumber(raw) {
            throw Error("INI value [" . section . "] " . key . " must be numeric")
        }
        value := raw + 0
    }
    if minimum != "" && value < minimum {
        throw Error("INI value [" . section . "] " . key . " is below its minimum")
    }
    if maximum != "" && value > maximum {
        throw Error("INI value [" . section . "] " . key . " is above its maximum")
    }
    return value
}


ReadInteger(section, key, defaultValue := "__GAME_TEST_REQUIRED__", minimum := "", maximum := "") {
    value := ReadNumber(section, key, defaultValue, minimum, maximum)
    if value != Round(value) {
        throw Error("INI value [" . section . "] " . key . " must be an integer")
    }
    return Round(value)
}


ReadBoolean(section, key, defaultValue := false) {
    raw := StrLower(ReadText(section, key, defaultValue ? "1" : "0"))
    if raw = "1" || raw = "true" || raw = "yes" {
        return true
    }
    if raw = "0" || raw = "false" || raw = "no" {
        return false
    }
    throw Error("INI value [" . section . "] " . key . " must be 0/1 or true/false")
}


ResolvePlanPath(path) {
    global gPlanDirectory
    path := Trim(path)
    if RegExMatch(path, "i)^[a-z]:\\") || SubStr(path, 1, 2) = "\\" {
        return path
    }
    return gPlanDirectory . "\" . path
}


ValidateSingleKey(key, label) {
    key := Trim(key)
    if !RegExMatch(key, "i)^(?:[a-z0-9]|f(?:[1-9]|1[0-9]|2[0-4])|esc|escape|tab|enter|space)$") {
        throw Error("Profile." . label . " must name exactly one safe keyboard key")
    }
    return key
}


ExecuteStep(index, step) {
    action := step["Action"]
    switch action {
        case "focus_game":
            FocusGame()
            return Outcome("D2R window focused and geometry verified")
        case "select_character":
            SelectCharacter()
            return Outcome("Configured test character selected")
        case "enter_game":
            EnterGame()
            return Outcome("Offline Play sequence sent")
        case "wait_for_in_game":
            WaitForState("IN_GAME", "InGame", gProfile["InGameTimeoutMs"])
            return Outcome("IN_GAME probe confirmed")
        case "open_inventory":
            OpenInventory()
            return Outcome("Inventory-open probe confirmed")
        case "capture":
            path := CaptureCheckpoint(index, step["Name"])
            return Outcome("D2R client checkpoint captured", path)
        case "close_inventory":
            CloseInventory()
            return Outcome("Inventory-close transition confirmed")
        case "save_and_exit":
            SaveAndExit()
            return Outcome("Save and Exit input sequence sent")
        case "wait_for_character_select":
            WaitForState("CHARACTER_SELECT", "CharacterSelect", gProfile["SaveExitTimeoutMs"])
            return Outcome("CHARACTER_SELECT probe confirmed after Save and Exit")
        default:
            throw RunnerFailure("Unsupported runner action: " . action, "failed")
    }
}


Outcome(message, artifactPath := "") {
    return Map("Message", message, "ArtifactPath", artifactPath)
}


FocusGame() {
    global gProfile

    started := A_TickCount
    loop {
        matches := GetMatchingWindows()
        if matches.Length > 1 {
            throw RunnerFailure("More than one allowlisted D2R window has the exact configured title", "failed")
        }
        if matches.Length = 1 {
            info := ValidateTargetWindow(matches[1], false, false)
            WinActivate("ahk_id " . info["Hwnd"])

            focusStarted := A_TickCount
            loop {
                if WinActive("ahk_id " . info["Hwnd"]) {
                    Sleep(gProfile["FocusSettleMs"])
                    try {
                        ValidateTargetWindow(info["Hwnd"], true, true)
                        return
                    } catch as err {
                        if !InStr(err.Message, "D2R client geometry ") {
                            throw err
                        }
                    }
                }
                if TickDuration(focusStarted) >= gProfile["WindowTimeoutMs"] {
                    throw RunnerFailure("Timed out while activating the D2R window", "failed")
                }
                Sleep(gProfile["PollIntervalMs"])
            }
        }

        if !AnyAllowedProcessAlive() && TickDuration(started) >= gProfile["WindowTimeoutMs"] {
            throw RunnerFailure("No configured D2R process or window became available", "failed")
        }
        if TickDuration(started) >= gProfile["WindowTimeoutMs"] {
            throw RunnerFailure("Configured D2R process exists but its exact window was not found", "failed")
        }
        Sleep(gProfile["PollIntervalMs"])
    }
}


SelectCharacter() {
    global gProfile

    if ProbeConfigured("CharacterSelect") {
        WaitForState("CHARACTER_SELECT", "CharacterSelect", gProfile["CharacterSelectTimeoutMs"])
    }
    info := RequireSafeTarget(true)

    templatePath := gProfile["CharacterTemplatePath"]
    if templatePath != "" {
        if !FileExist(templatePath) {
            throw RunnerFailure("Character template is missing: " . templatePath, "inconclusive")
        }
        match := SearchTemplate(info, templatePath,
            gProfile["CharacterSearchX1"], gProfile["CharacterSearchY1"],
            gProfile["CharacterSearchX2"], gProfile["CharacterSearchY2"],
            gProfile["CharacterTemplateVariation"])
        if !match["Found"] {
            throw RunnerFailure("Configured test character image was not found", "failed")
        }
        clickX := match["X"] + Round(gProfile["CharacterClickOffsetX"] * info["Width"])
        clickY := match["Y"] + Round(gProfile["CharacterClickOffsetY"] * info["Height"])
        ClickClientScreenPoint(clickX, clickY, "character-template")
    } else if gProfile["CharacterFallbackConfigured"] {
        ClickNormalized(gProfile["CharacterFallbackX"], gProfile["CharacterFallbackY"], "character-fallback")
    } else {
        throw RunnerFailure("No CharacterTemplatePath or calibrated CharacterFallbackX/Y is configured", "inconclusive")
    }
    Sleep(gProfile["AfterCharacterClickMs"])
}


EnterGame() {
    global gProfile

    if !ProbeConfigured("CharacterSelect") {
        throw RunnerFailure("CharacterSelect probe is required before Play input", "inconclusive")
    }
    WaitForState("CHARACTER_SELECT", "CharacterSelect", gProfile["CharacterSelectTimeoutMs"])
    ClickNormalized(gProfile["PlayX"], gProfile["PlayY"], "play")
    Sleep(gProfile["AfterPlayClickMs"])

    if gProfile["DifficultyEnabled"] {
        ClickNormalized(gProfile["DifficultyX"], gProfile["DifficultyY"], "difficulty")
        Sleep(gProfile["AfterDifficultyClickMs"])
    }
}


OpenInventory() {
    global gProfile

    if !ProbeConfigured("InGame") {
        throw RunnerFailure("InGame probe is required before inventory input", "inconclusive")
    }
    if !ProbeIsVisible("InGame") {
        throw RunnerFailure("IN_GAME was not confirmed before opening inventory", "failed")
    }
    if !ProbeConfigured("Inventory") {
        throw RunnerFailure("Inventory probe is required to confirm inventory-open", "inconclusive")
    }
    if ProbeIsVisible("Inventory") {
        throw RunnerFailure("Inventory was already open before the open_inventory step", "failed")
    }

    SendGameKey(gProfile["InventoryKey"], "open-inventory")
    Sleep(gProfile["AfterInventoryKeyMs"])
    WaitForProbePresence("Inventory", true, gProfile["InventoryTimeoutMs"],
        "Inventory did not become visible after the configured key")
}


CloseInventory() {
    global gProfile

    if !ProbeConfigured("Inventory") {
        throw RunnerFailure("Inventory probe is required to confirm inventory-close", "inconclusive")
    }
    if !ProbeIsVisible("Inventory") {
        throw RunnerFailure("Inventory was not open before the close_inventory step", "failed")
    }

    SendGameKey(gProfile["InventoryKey"], "close-inventory")
    Sleep(gProfile["AfterInventoryKeyMs"])
    WaitForProbePresence("Inventory", false, gProfile["InventoryTimeoutMs"],
        "Inventory remained visible after the configured key")

    if ProbeConfigured("InGame") && !ProbeIsVisible("InGame") {
        throw RunnerFailure("Inventory disappeared but IN_GAME could not be reconfirmed", "failed")
    }
}


SaveAndExit() {
    global gProfile

    state := GetState()
    if state = "IN_GAME" {
        SendGameKey(gProfile["EscapeKey"], "open-escape-menu")
        Sleep(gProfile["AfterEscapeMs"])
        WaitForState("ESC_MENU", "EscMenu", gProfile["InventoryTimeoutMs"])
    } else if state != "ESC_MENU" {
        throw RunnerFailure("Save and Exit requires a confirmed IN_GAME or ESC_MENU state; got " . state, "failed")
    }

    ClickNormalized(gProfile["SaveAndExitX"], gProfile["SaveAndExitY"], "save-and-exit")
    Sleep(gProfile["AfterSaveExitClickMs"])
}


WaitForState(expectedState, probeName, timeoutMs) {
    global gProfile

    if !ProbeConfigured(probeName) {
        throw RunnerFailure(probeName . " probe is not configured, so " . expectedState . " cannot be confirmed", "inconclusive")
    }

    started := A_TickCount
    loop {
        info := RequireSafeTarget(true)
        if ProbeIsVisibleWithInfo(probeName, info) {
            return
        }
        if TickDuration(started) >= timeoutMs {
            throw RunnerFailure("Timed out waiting for state " . expectedState, "failed")
        }
        Sleep(gProfile["PollIntervalMs"])
    }
}


WaitForProbePresence(probeName, shouldBePresent, timeoutMs, failureMessage) {
    global gProfile

    if !ProbeConfigured(probeName) {
        throw RunnerFailure(probeName . " probe is not configured", "inconclusive")
    }

    started := A_TickCount
    consecutiveMatches := 0
    loop {
        info := RequireSafeTarget(true)
        visible := ProbeIsVisibleWithInfo(probeName, info)
        matchesExpectation := shouldBePresent ? visible : !visible
        if matchesExpectation {
            consecutiveMatches += 1
            if consecutiveMatches >= 2 {
                return
            }
        } else {
            consecutiveMatches := 0
        }
        if TickDuration(started) >= timeoutMs {
            throw RunnerFailure(failureMessage, "failed")
        }
        Sleep(gProfile["PollIntervalMs"])
    }
}


ProbeConfigured(name) {
    global gProbes
    return gProbes.Has(name) && gProbes[name]["Path"] != ""
}


ProbeIsVisible(name) {
    info := RequireSafeTarget(true)
    return ProbeIsVisibleWithInfo(name, info)
}


ProbeIsVisibleWithInfo(name, info) {
    global gProbes

    if !ProbeConfigured(name) {
        throw RunnerFailure(name . " probe is not configured", "inconclusive")
    }
    probe := gProbes[name]
    if !FileExist(probe["Path"]) {
        throw RunnerFailure(name . " probe template is missing: " . probe["Path"], "inconclusive")
    }
    match := SearchTemplate(info, probe["Path"], probe["X1"], probe["Y1"],
        probe["X2"], probe["Y2"], probe["Variation"])
    return match["Found"]
}


SearchTemplate(info, templatePath, normalizedX1, normalizedY1, normalizedX2, normalizedY2, variation) {
    rectangle := ResolveSearchRectangle(info, normalizedX1, normalizedY1, normalizedX2, normalizedY2)
    foundX := 0
    foundY := 0
    options := "*" . variation . " " . templatePath
    try {
        found := ImageSearch(&foundX, &foundY,
            rectangle["X1"], rectangle["Y1"], rectangle["X2"], rectangle["Y2"], options)
    } catch as err {
        throw RunnerFailure("ImageSearch failed for " . templatePath . ": " . err.Message, "inconclusive")
    }
    return Map("Found", found, "X", foundX, "Y", foundY)
}


ResolveSearchRectangle(info, x1, y1, x2, y2) {
    left := info["X"] + Floor(x1 * (info["Width"] - 1))
    top := info["Y"] + Floor(y1 * (info["Height"] - 1))
    right := info["X"] + Floor(x2 * (info["Width"] - 1))
    bottom := info["Y"] + Floor(y2 * (info["Height"] - 1))
    if left > right || top > bottom {
        throw RunnerFailure("Resolved image-search rectangle is invalid", "failed")
    }
    return Map("X1", left, "Y1", top, "X2", right, "Y2", bottom)
}


ClickNormalized(normalizedX, normalizedY, label) {
    info := RequireSafeTarget(true)
    x := info["X"] + Round(normalizedX * (info["Width"] - 1))
    y := info["Y"] + Round(normalizedY * (info["Height"] - 1))
    ClickClientScreenPoint(x, y, label)
}


ClickClientScreenPoint(x, y, label) {
    global gProfile

    info := RequireSafeTarget(true)
    AssertPointStrictlyInside(info, x, y, label)

    MouseMove(x, y, gProfile["MouseMoveSpeed"])
    infoAfterMove := RequireSafeTarget(true)
    if infoAfterMove["Hwnd"] != info["Hwnd"]
        || infoAfterMove["X"] != info["X"] || infoAfterMove["Y"] != info["Y"]
        || infoAfterMove["Width"] != info["Width"] || infoAfterMove["Height"] != info["Height"] {
        throw RunnerFailure("D2R target changed between mouse move and click; click cancelled", "failed")
    }
    AssertPointStrictlyInside(infoAfterMove, x, y, label)

    virtualLeft := SysGet(76)
    virtualTop := SysGet(77)
    virtualWidth := SysGet(78)
    virtualHeight := SysGet(79)
    if x < virtualLeft || y < virtualTop
        || x >= virtualLeft + virtualWidth || y >= virtualTop + virtualHeight {
        throw RunnerFailure("Refused " . label . " click outside the virtual desktop", "failed")
    }

    actualX := 0
    actualY := 0
    MouseGetPos(&actualX, &actualY)
    if actualX != x || actualY != y {
        throw RunnerFailure("Mouse did not reach the exact safe D2R point; click cancelled", "failed")
    }
    packedPoint := (y << 32) | (x & 0xFFFFFFFF)
    pointWindow := DllCall("user32\WindowFromPoint", "Int64", packedPoint, "Ptr")
    rootWindow := pointWindow ? DllCall("user32\GetAncestor", "Ptr", pointWindow, "UInt", 2, "Ptr") : 0
    if !rootWindow || rootWindow != infoAfterMove["Hwnd"] {
        throw RunnerFailure("Another window or overlay covers the D2R click point; click cancelled", "failed")
    }
    Click(x, y)
    RecordInput("click:" . label . "@" . x . "," . y)
    Sleep(gProfile["InputDurationMs"])
}


AssertPointStrictlyInside(info, x, y, label) {
    if x <= info["X"] || y <= info["Y"]
        || x >= info["X"] + info["Width"] - 1
        || y >= info["Y"] + info["Height"] - 1 {
        throw RunnerFailure("Refused " . label . " click outside the strict D2R client interior", "failed")
    }
}


SendGameKey(key, label) {
    global gProfile

    info := RequireSafeTarget(true)
    keySpec := "{" . key . "}"
    if !WinActive("ahk_id " . info["Hwnd"]) {
        throw RunnerFailure("D2R lost focus immediately before key input; key cancelled", "failed")
    }
    SendEvent(keySpec)
    RecordInput("key:" . label . "=" . key)
    Sleep(gProfile["InputDurationMs"])
}


RequireSafeTarget(requireActive := true) {
    if !AnyAllowedProcessAlive() {
        throw RunnerFailure("No allowlisted D2R process exists", "failed")
    }
    matches := GetMatchingWindows()
    if matches.Length = 0 {
        throw RunnerFailure("No allowlisted process owns a window with the exact configured D2R title", "failed")
    }
    if matches.Length > 1 {
        throw RunnerFailure("Multiple allowlisted windows match the exact configured D2R title", "failed")
    }
    return ValidateTargetWindow(matches[1], requireActive)
}


ValidateTargetWindow(hwnd, requireActive := true, requireExpectedGeometry := true) {
    global gRunner, gProfile

    try title := WinGetTitle("ahk_id " . hwnd)
    catch {
        throw RunnerFailure("D2R target window no longer exists", "failed")
    }
    if title != gRunner["WindowTitle"] {
        throw RunnerFailure("Target window title changed; operation cancelled", "failed")
    }

    try processName := StrLower(WinGetProcessName("ahk_id " . hwnd))
    catch {
        throw RunnerFailure("Cannot verify the target window process", "failed")
    }
    if !IsAllowedProcessName(processName) {
        throw RunnerFailure("Exact-title window is not owned by an allowlisted executable", "failed")
    }
    try processPath := WinGetProcessPath("ahk_id " . hwnd)
    catch {
        throw RunnerFailure("Cannot verify the target executable path", "failed")
    }
    trustedRoot := StrLower(gRunner["WorkingDirectory"])
    normalizedProcessPath := StrLower(processPath)
    if SubStr(normalizedProcessPath, 1, StrLen(trustedRoot) + 1) != trustedRoot . "\" {
        throw RunnerFailure("Allowlisted process name is outside the configured D2R directory", "failed")
    }
    if requireActive && !WinActive("ahk_id " . hwnd) {
        throw RunnerFailure("D2R is not the active window; input block cancelled", "failed")
    }

    x := 0
    y := 0
    width := 0
    height := 0
    try WinGetClientPos(&x, &y, &width, &height, "ahk_id " . hwnd)
    catch as err {
        throw RunnerFailure("Cannot read D2R client geometry: " . err.Message, "failed")
    }
    if width <= 2 || height <= 2 {
        throw RunnerFailure("D2R client rectangle is empty", "failed")
    }
    if requireExpectedGeometry && (Abs(width - gProfile["ExpectedWidth"]) > gProfile["GeometryTolerancePixels"]
        || Abs(height - gProfile["ExpectedHeight"]) > gProfile["GeometryTolerancePixels"]) {
        throw RunnerFailure("D2R client geometry " . width . "x" . height
            . " does not match expected " . gProfile["ExpectedWidth"] . "x"
            . gProfile["ExpectedHeight"] . " +/- " . gProfile["GeometryTolerancePixels"] . " px", "failed")
    }
    return Map("Hwnd", hwnd, "X", x, "Y", y, "Width", width, "Height", height)
}


GetMatchingWindows() {
    global gRunner

    matches := []
    try candidates := WinGetList(gRunner["WindowTitle"])
    catch {
        return matches
    }
    for hwnd in candidates {
        try {
            if WinGetTitle("ahk_id " . hwnd) != gRunner["WindowTitle"] {
                continue
            }
            processName := StrLower(WinGetProcessName("ahk_id " . hwnd))
            if IsAllowedProcessName(processName) {
                matches.Push(hwnd)
            }
        }
    }
    return matches
}


IsAllowedProcessName(processName) {
    global gRunner
    for allowedName in gRunner["ProcessNames"] {
        if processName = allowedName {
            return true
        }
    }
    return false
}


AnyAllowedProcessAlive() {
    global gRunner
    for processName in gRunner["ProcessNames"] {
        try {
            if ProcessExist(processName) {
                return true
            }
        }
    }
    return false
}


GetState() {
    if !AnyAllowedProcessAlive() {
        return "NOT_RUNNING"
    }
    matches := GetMatchingWindows()
    if matches.Length = 0 {
        return "LOADING"
    }
    if matches.Length > 1 {
        return "FAILED"
    }

    try info := ValidateTargetWindow(matches[1], false)
    catch {
        return "FAILED"
    }

    try {
        if ProbeConfigured("CharacterSelect") && ProbeIsVisibleWithInfo("CharacterSelect", info) {
            return "CHARACTER_SELECT"
        }
        if ProbeConfigured("EscMenu") && ProbeIsVisibleWithInfo("EscMenu", info) {
            return "ESC_MENU"
        }
        if ProbeConfigured("InGame") && ProbeIsVisibleWithInfo("InGame", info) {
            return "IN_GAME"
        }
    } catch {
        return "FAILED"
    }
    return "LOADING"
}


SafeGetState() {
    try {
        return GetState()
    } catch {
        return "FAILED"
    }
}


CaptureCheckpoint(index, requestedName) {
    name := requestedName = "" ? "capture" : requestedName
    path := BuildArtifactPath(Format("{:03}", index) . "-" . SanitizeFileName(name) . ".png")
    info := RequireSafeTarget(true)
    LogEvent(index, "capture_started", "completed", SafeGetState(), 0,
        "D2R client capture started")
    ConsoleWrite("CaptureClientPng starting: " . path)
    CaptureClientPng(info, path, false)
    ConsoleWrite("CaptureClientPng returned: " . path)
    return path
}


TryFailureCapture(index, action) {
    path := BuildArtifactPath("failure-" . Format("{:03}", index) . "-" . SanitizeFileName(action) . ".png")
    started := A_TickCount
    try {
        matches := GetMatchingWindows()
        if matches.Length != 1 {
            throw RunnerFailure("No unique allowlisted D2R window is available for failure capture", "inconclusive")
        }
        isActive := !!WinActive("ahk_id " . matches[1])
        info := ValidateTargetWindow(matches[1], false, false)
        CaptureClientPng(info, path, !isActive)
        LogEvent(index, "failure_capture", "completed", SafeGetState(), TickDuration(started),
            "Failure screenshot captured before recovery", path)
        return path
    } catch as err {
        LogEvent(index, "failure_capture", "inconclusive", SafeGetState(), TickDuration(started),
            "Failure screenshot unavailable: " . err.Message)
        return ""
    }
}


CaptureClientPng(info, path, usePrintWindow) {
    hdcScreen := 0
    hdcWindow := 0
    hdcMemory := 0
    bitmap := 0
    oldBitmap := 0
    gdipBitmap := 0
    gdipToken := 0

    try {
        hdcScreen := DllCall("user32\GetDC", "Ptr", 0, "Ptr")
        if !hdcScreen {
            throw Error("GetDC failed")
        }
        hdcMemory := DllCall("gdi32\CreateCompatibleDC", "Ptr", hdcScreen, "Ptr")
        if !hdcMemory {
            throw Error("CreateCompatibleDC failed")
        }
        bitmap := DllCall("gdi32\CreateCompatibleBitmap", "Ptr", hdcScreen,
            "Int", info["Width"], "Int", info["Height"], "Ptr")
        if !bitmap {
            throw Error("CreateCompatibleBitmap failed")
        }
        oldBitmap := DllCall("gdi32\SelectObject", "Ptr", hdcMemory, "Ptr", bitmap, "Ptr")
        if !oldBitmap {
            throw Error("SelectObject failed")
        }

        captured := false
        if usePrintWindow {
            captured := !!DllCall("user32\PrintWindow", "Ptr", info["Hwnd"], "Ptr", hdcMemory,
                "UInt", 3, "Int")
            if !captured {
                hdcWindow := DllCall("user32\GetDC", "Ptr", info["Hwnd"], "Ptr")
                if hdcWindow {
                    captured := !!DllCall("gdi32\BitBlt", "Ptr", hdcMemory, "Int", 0, "Int", 0,
                        "Int", info["Width"], "Int", info["Height"], "Ptr", hdcWindow,
                        "Int", 0, "Int", 0, "UInt", 0x40CC0020, "Int")
                }
            }
        } else {
            captured := !!DllCall("gdi32\BitBlt", "Ptr", hdcMemory, "Int", 0, "Int", 0,
                "Int", info["Width"], "Int", info["Height"], "Ptr", hdcScreen,
                "Int", info["X"], "Int", info["Y"], "UInt", 0x40CC0020, "Int")
        }
        if !captured {
            throw Error("PrintWindow/BitBlt failed")
        }

        startupInput := Buffer(A_PtrSize = 8 ? 24 : 16, 0)
        NumPut("UInt", 1, startupInput, 0)
        tokenBuffer := Buffer(A_PtrSize, 0)
        status := DllCall("gdiplus\GdiplusStartup", "Ptr", tokenBuffer.Ptr,
            "Ptr", startupInput.Ptr, "Ptr", 0, "UInt")
        if status != 0 {
            throw Error("GdiplusStartup status " . status)
        }
        gdipToken := NumGet(tokenBuffer, 0, "UPtr")
        bitmapBuffer := Buffer(A_PtrSize, 0)
        status := DllCall("gdiplus\GdipCreateBitmapFromHBITMAP", "Ptr", bitmap, "Ptr", 0,
            "Ptr", bitmapBuffer.Ptr, "UInt")
        gdipBitmap := NumGet(bitmapBuffer, 0, "Ptr")
        if status != 0 || !gdipBitmap {
            throw Error("GdipCreateBitmapFromHBITMAP status " . status)
        }
        pngClsid := Buffer(16, 0)
        status := DllCall("ole32\CLSIDFromString",
            "WStr", "{557CF406-1A04-11D3-9A73-0000F81EF32E}", "Ptr", pngClsid.Ptr, "Int")
        if status != 0 {
            throw Error("CLSIDFromString status " . status)
        }
        status := DllCall("gdiplus\GdipSaveImageToFile", "Ptr", gdipBitmap,
            "WStr", path, "Ptr", pngClsid.Ptr, "Ptr", 0, "UInt")
        if status != 0 {
            throw Error("GdipSaveImageToFile status " . status)
        }
        ConsoleWrite("CaptureClientPng image data saved")
    } catch as err {
        throw RunnerFailure("D2R client capture failed: " . err.Message, "inconclusive")
    } finally {
        if gdipBitmap {
            ConsoleWrite("CaptureClientPng disposing GDI+ image")
            DllCall("gdiplus\GdipDisposeImage", "Ptr", gdipBitmap)
            ConsoleWrite("CaptureClientPng disposed GDI+ image")
        }
        ; The runner is short-lived and captures only bounded checkpoints. The
        ; installed AHK/GDI+ combination terminates inside GdiplusShutdown, so
        ; the process owns this token until ExitApp and the OS releases it.
        if oldBitmap && hdcMemory {
            ConsoleWrite("CaptureClientPng restoring selected bitmap")
            DllCall("gdi32\SelectObject", "Ptr", hdcMemory, "Ptr", oldBitmap, "Ptr")
            ConsoleWrite("CaptureClientPng restored selected bitmap")
        }
        if bitmap {
            ConsoleWrite("CaptureClientPng deleting bitmap")
            DllCall("gdi32\DeleteObject", "Ptr", bitmap, "Int")
            ConsoleWrite("CaptureClientPng deleted bitmap")
        }
        if hdcMemory {
            ConsoleWrite("CaptureClientPng deleting memory DC")
            DllCall("gdi32\DeleteDC", "Ptr", hdcMemory, "Int")
            ConsoleWrite("CaptureClientPng deleted memory DC")
        }
        if hdcWindow {
            ConsoleWrite("CaptureClientPng releasing window DC")
            DllCall("user32\ReleaseDC", "Ptr", info["Hwnd"], "Ptr", hdcWindow, "Int")
            ConsoleWrite("CaptureClientPng released window DC")
        }
        if hdcScreen {
            ConsoleWrite("CaptureClientPng releasing screen DC")
            DllCall("user32\ReleaseDC", "Ptr", 0, "Ptr", hdcScreen, "Int")
            ConsoleWrite("CaptureClientPng released screen DC")
        }
    }
    ConsoleWrite("CaptureClientPng cleanup completed")
}


TryRecovery(stepIndex) {
    global gProfile

    state := SafeGetState()
    if state != "IN_GAME" && state != "ESC_MENU" {
        return false
    }

    matches := GetMatchingWindows()
    if matches.Length != 1 || !WinActive("ahk_id " . matches[1]) {
        return false
    }

    started := A_TickCount
    try {
        ValidateTargetWindow(matches[1], true)
        if state = "IN_GAME" {
            SendGameKey(gProfile["EscapeKey"], "recovery-open-escape-menu")
            Sleep(gProfile["AfterEscapeMs"])
            WaitForState("ESC_MENU", "EscMenu", gProfile["InventoryTimeoutMs"])
        }
        ClickNormalized(gProfile["SaveAndExitX"], gProfile["SaveAndExitY"], "recovery-save-and-exit")
        Sleep(gProfile["AfterSaveExitClickMs"])
        WaitForState("CHARACTER_SELECT", "CharacterSelect", gProfile["SaveExitTimeoutMs"])
        LogEvent(stepIndex, "recovery_save_and_exit", "completed", SafeGetState(),
            TickDuration(started), "Single recovery attempt returned to character selection")
    } catch as err {
        status := HasProp(err, "Status") && err.Status = "inconclusive" ? "inconclusive" : "failed"
        LogEvent(stepIndex, "recovery_save_and_exit", status, SafeGetState(),
            TickDuration(started), "Single recovery attempt failed: " . err.Message)
    }
    return true
}


BuildArtifactPath(fileName) {
    global gRunner
    return RTrim(gRunner["RunDirectory"], "\/") . "\" . fileName
}


SanitizeFileName(value) {
    value := RegExReplace(Trim(value), "[^A-Za-z0-9._-]+", "-")
    value := Trim(value, ".-_")
    return value = "" ? "capture" : SubStr(value, 1, 80)
}


RecordInput(description) {
    global gRecentInputs
    gRecentInputs.Push(description)
    while gRecentInputs.Length > 10 {
        gRecentInputs.RemoveAt(1)
    }
}


RecentInputSuffix() {
    global gRecentInputs
    if gRecentInputs.Length = 0 {
        return "; recentInputs=none"
    }
    text := ""
    for index, item in gRecentInputs {
        text .= (index = 1 ? "" : " > ") . item
    }
    return "; recentInputs=" . text
}


LogEvent(stepIndex, action, status, state, durationMs, message, artifactPath := "") {
    global gRunner, gInitialized

    if !gInitialized {
        return
    }
    artifactJson := artifactPath = "" ? "null" : JsonQuote(artifactPath)
    line := "{" . JsonQuote("timestamp") . ":" . JsonQuote(UtcTimestamp())
        . "," . JsonQuote("stepIndex") . ":" . Round(stepIndex)
        . "," . JsonQuote("action") . ":" . JsonQuote(action)
        . "," . JsonQuote("status") . ":" . JsonQuote(status)
        . "," . JsonQuote("state") . ":" . JsonQuote(state)
        . "," . JsonQuote("durationMs") . ":" . Round(durationMs)
        . "," . JsonQuote("message") . ":" . JsonQuote(message)
        . "," . JsonQuote("artifactPath") . ":" . artifactJson . "}"
    FileAppend(line . "`n", gRunner["EventLogPath"], "UTF-8-RAW")
}


JsonQuote(value) {
    value := value . ""
    value := StrReplace(value, "\", "\\")
    value := StrReplace(value, Chr(34), "\" . Chr(34))
    value := StrReplace(value, "`b", "\b")
    value := StrReplace(value, "`f", "\f")
    value := StrReplace(value, "`r", "\r")
    value := StrReplace(value, "`n", "\n")
    value := StrReplace(value, "`t", "\t")
    return Chr(34) . value . Chr(34)
}


UtcTimestamp() {
    return FormatTime(A_NowUTC, "yyyy-MM-dd") . "T"
        . FormatTime(A_NowUTC, "HH:mm:ss") . "." . Format("{:03}", A_MSec) . "Z"
}


TickDuration(started) {
    elapsed := A_TickCount - started
    return elapsed < 0 ? elapsed + 0x100000000 : elapsed
}


ConsoleWrite(message) {
    try FileAppend(message . "`n", "*", "UTF-8-RAW")
}


EmergencyStop(*) {
    global gInitialized, gCurrentStep, gCurrentAction

    if gInitialized {
        try LogEvent(gCurrentStep, "emergency_stop", "failed", SafeGetState(), 0,
            "Pause hotkey requested immediate runner abort during " . gCurrentAction)
    }
    ConsoleWrite("GameTestRunner aborted by Pause hotkey")
    ExitApp(130)
}


ReleaseInstanceMutex(*) {
    global gInstanceMutex
    if gInstanceMutex {
        DllCall("kernel32\ReleaseMutex", "Ptr", gInstanceMutex)
        DllCall("kernel32\CloseHandle", "Ptr", gInstanceMutex)
        gInstanceMutex := 0
    }
}
