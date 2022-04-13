#include "ImGui/imgui.h" // ImGui functions
#include "ImGui/backends/imgui_impl_dx9.h" // ImGui backend
#include "ImGui/backends/imgui_impl_win32.h" // ImGui backend
#include "ImGui/misc/cpp/imgui_stdlib.h" // inputText std::string as buffer

#include <d3d9.h> // Used for graphics device to display Dear ImGui
#include <tchar.h> // _T() ???
#include "DB.h" // My database library
#include "Language.h"

//////////
// TODO //
//////////

/*
* 
* BUGS:
* editing a device moves it to ID=2016 which breaks loading the full data
* 
* TODO:
* Implement admin adding a device the normal way
* Implement admin removing a device
* 
*/

//////////////
// TODO END //
//////////////

static const ImGuiTableFlags tableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders;

enum class YesNoAny {
    YES,
    NO,
    ANY
};

enum class CurrentScreen {
    FILTER,
    DEVICE,
    ADMIN,
    SERVICE
};

enum class AdminScreen {
    NONE,
    EDIT,
    ADD,
    DECOMMISSION
};

namespace RepeatCheck {
    static const int NEVER = 0,
                     DAILY = 1,
                     WEEKLY = 2,
                     MONTHLY = 3,
                     QUARTERLY = 4,
                     BIYEARLY = 5,
                     YEARLY = 6,
                     HALFDECENIUM = 7,
                     DECENIUM = 8;
}

static const std::string emptyString = "";

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::string frequencyNotation[9] = { // switch from this to language based notations
    "%FREQ_NO_CHECKS%",
    "%FREQ_DAILY%",
    "%FREQ_WEEKLY%",
    "%FREQ_MONTHLY%",
    "%FREQ_QUARTERLY%",
    "%FREQ_BI_YEARLY%",
    "%FREQ_YEARLY%",
    "%FREQ_HALF_DECENIUM%",
    "%FREQ_DECENIUM%"
};

// Styling the program
static const float DARK_FACTOR = 0.7f;
// colors
static const ImVec4 GREEN = ImVec4(0.0f, 0.5f, 0.3f, 1.0f);
static const ImVec4 DARK_GREEN = ImVec4(0.0f, 0.5f * DARK_FACTOR, 0.3f * DARK_FACTOR, 1.0f);
static const ImVec4 LIGHT_GREEN = ImVec4(0.0f, 0.5f / DARK_FACTOR, 0.3f / DARK_FACTOR, 1.0f);
static const ImVec4 WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 GRAY = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

static const time_t frequencyDateOffset[9]{
    Date::getOffset( 0, 0, 0 + 1).t,
    Date::getOffset( 0, 0, 1 + 1).t,
    Date::getOffset( 0, 0, 7 + 1).t,
    Date::getOffset( 0, 1, 0 + 1).t,
    Date::getOffset( 0, 3, 0 + 1).t,
    Date::getOffset( 0, 6, 0 + 1).t,
    Date::getOffset( 1, 0, 0 + 1).t,
    Date::getOffset( 5, 0, 0 + 1).t,
    Date::getOffset(10, 0, 0 + 1).t
};

static void addDevice(int id) {
    DeviceData t = { id, "test" + std::to_string(id % 5), id % 2 ? true : false, "VERW" + std::to_string(id % 7), std::to_string(id % 8 + 40100), "device_type" + std::to_string(id % 7), 123, "companySupplier" + std::to_string(id % 8), "companyManufacturer" + std::to_string(id % 2), 0, 0, std::to_string(id % 15), "WetChemSpectro", "Admin", "Replacer", true, false, 0, 0, 0, 0, "ExtCheck", 0, 0, 0, "ContractDesc", 0, 0, 0.3f * (id % 200) + 20.0f};
    DB::addDevice(t, false);
};

// GUI macros
static inline void checkBox(const std::string label, bool& value, int& counter) {
    ImGui::PushID(counter);
    ImGui::Checkbox(label.c_str(), &value);
    ImGui::PopID();
    counter++;
}

static inline void checkBox(const std::string label, unsigned short& value, int& counter) {
    ImGui::PushID(counter);
    ImGui::Checkbox(label.c_str(), (bool*)&value);
    ImGui::PopID();
    counter++;
}

static inline void intInput(const std::string label, int& value, int& counter) {
    ImGui::PushID(counter);
    ImGui::InputInt(label.c_str(), &value);
    ImGui::PopID();
    counter++;
}

static inline void textInput(const std::string& label, std::string& value, int& counter) {
    ImGui::PushID(counter);
    ImGui::InputText(label.c_str(), &value);
    ImGui::PopID();
    counter++;
}

static inline void dateInput(Date& date, std::string& str, const char* label, int& counter) {
    Date::filterDateInput(str);
    if (str.size() == 0) { str = date.asString(); }
    else { date.fromStr(str); }
    textInput(label, str, counter);
};

static inline bool button(const std::string& label, int& counter) {
    ImGui::PushID(counter);
    bool output = ImGui::Button(label.c_str());
    ImGui::PopID();
    counter++;
    return output;
}

static inline bool selectable(const std::string& label, bool isActive, int& counter) {
    ImGui::PushID(counter);
    bool output = ImGui::Selectable(label.c_str(), isActive);
    ImGui::PopID();
    counter++;
    return output;
}

// Main code
int main(int, char**) {
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ABS AgroLab v0.1"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("ABS AgroLab v0.1"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
    RECT hwndSize;
    GetWindowRect(hwnd, &hwndSize);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    ///////////
    /// ABS ///
    ///////////

    FilterMenu filterMenu;
    DeviceMenu deviceMenu;
    Language language = Language();
    
    // Window bools
    bool done = false;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    CurrentScreen screen = CurrentScreen::FILTER;
    int windowWidth = 1280, windowHeight = 800;

    // Filters
    YesNoAny inUse = YesNoAny::ANY;
    YesNoAny requiresCheckup = YesNoAny::ANY;
    std::vector<std::string> locations, costPlaces;
    std::vector<unsigned short> locationFilter, costplaceFilter;

    // Rendering bools
    bool recommendReload = false;
    bool loadAttempt = false;
    bool reloadFilter = true;
    bool shouldAdd = false;
    bool displayedDevices = false;
    bool isEnabled; // for filter display
    bool canDisplay; // for filter display
    int currentID; // used for enabling buttons/checkboxes/inputs with same labels

    // Device screen
    int searchId = 0;

    // Admin screen
    int adminSearch = 0;
    AdminScreen adminScreen = AdminScreen::NONE;
    DeviceMenu adminDevice;
    DeviceMenu editDevice;
    EditDeviceDates adminEditDevice;
    EditDeviceDates adminAddDevice;

    // style ptr
    ImGuiStyle* style = &ImGui::GetStyle();
    // text
    style->Colors[ImGuiCol_Text] = WHITE;
    style->Colors[ImGuiCol_TextDisabled] = GRAY;
    style->Colors[ImGuiCol_TextSelectedBg] = GREEN;
    // tabs
    style->Colors[ImGuiCol_Tab] = DARK_GREEN;
    style->Colors[ImGuiCol_TabActive] = GREEN;
    style->Colors[ImGuiCol_TabHovered] = LIGHT_GREEN;
    // buttons
    style->Colors[ImGuiCol_Button] = DARK_GREEN;
    style->Colors[ImGuiCol_ButtonActive] = GREEN;
    style->Colors[ImGuiCol_ButtonHovered] = LIGHT_GREEN;
    // checkbox
    style->Colors[ImGuiCol_CheckMark] = LIGHT_GREEN;
    // table
    style->Colors[ImGuiCol_Header] = DARK_GREEN;
    style->Colors[ImGuiCol_HeaderHovered] = LIGHT_GREEN;
    style->Colors[ImGuiCol_HeaderActive] = GREEN;
    style->Colors[ImGuiCol_TableHeaderBg] = DARK_GREEN;
    // scrollbar
    style->Colors[ImGuiCol_ScrollbarGrab] = DARK_GREEN;
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = LIGHT_GREEN;
    style->Colors[ImGuiCol_ScrollbarGrabActive] = GREEN;
    // background of checkbox & inputs
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.5f * DARK_FACTOR * DARK_FACTOR, 0.3f * DARK_FACTOR * DARK_FACTOR, 1.0f); // DARK_DARK_GREEN
    style->Colors[ImGuiCol_FrameBgHovered] = GREEN;
    style->Colors[ImGuiCol_FrameBgActive] = DARK_GREEN;

    // TMP device add value
    int current = 0; // used in admin testing section as device id

    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) { done = true; }
        }
        if (done) { break; }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Reload if needed
        if (reloadFilter) {
            filterMenu.reload();
            costPlaces.clear();
            locations.clear();
            locations.push_back(language.getTranslation("%FILTER_SELECT_ALL%").c_str());
            costPlaces.push_back(language.getTranslation("%FILTER_SELECT_ALL%").c_str());
            current = filterMenu.id.size();
            // locations filter
            for (int i = 0; i < filterMenu.id.size(); i++) {
                shouldAdd = true;
                for (int j = 0; j < locations.size(); j++) {
                    if (locations[j].compare(filterMenu.location[i]) == 0) {
                        shouldAdd = false; break;
                    }
                }
                if (shouldAdd) { locations.push_back(filterMenu.location[i]); }
            }
            // costplace filter
            for (int i = 0; i < filterMenu.id.size(); i++) {
                shouldAdd = true;
                for (int j = 0; j < costPlaces.size(); j++) {
                    if (costPlaces[j].compare(filterMenu.costplace[i]) == 0) {
                        shouldAdd = false; break;
                    }
                }
                if (shouldAdd) { costPlaces.push_back(filterMenu.costplace[i]); }
            }

            locationFilter.clear();
            costplaceFilter.clear();
            while (locationFilter.size() < locations.size()) { locationFilter.push_back(1); }
            while (costplaceFilter.size() < costPlaces.size()) { costplaceFilter.push_back(1); }
            reloadFilter = false;
        }
        
        {
            Date today;
            currentID = 0;

            // Set window data
            GetWindowRect(hwnd, &hwndSize);
            windowHeight = hwndSize.bottom - hwndSize.top;
            windowWidth = hwndSize.right - hwndSize.left;

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(windowWidth-15, windowHeight-38)); // change to size of window
            ImGui::Begin("ABS", &done, flags | ImGuiWindowFlags_NoTitleBar);

            if (ImGui::BeginTabBar("TABS", NULL)) {
                if (ImGui::BeginTabItem(language.getTranslation("%FILTER%").c_str(), nullptr, NULL)) {
                    ////////////////////
                    // Reload message //
                    ////////////////////

                    if (screen != CurrentScreen::FILTER) { recommendReload = true; }
                    if (recommendReload) {
                        ImGui::Text(language.getTranslation("%RELOAD_MESSAGE%").c_str());
                        if (button(language.getTranslation("%PRESS_RELOAD%").c_str(), currentID)) { recommendReload = false; reloadFilter = true; }
                        ImGui::Separator();
                    }

                    /////////////
                    // Filters //
                    /////////////

                    if (ImGui::BeginTable("DevicesFilter", 4, tableFlags)) {
                        //ImGui::TableSetupColumn("Device ID");
                        //ImGui::TableSetupColumn("Device name");
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_IN_USE%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_LOCATION%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_COSTPLACE%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_CHECKUP%").c_str());
                        ImGui::TableHeadersRow();

                        // Display all the devices in the reference table
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                        // In use
                        ImGui::TableNextColumn();
                        if (ImGui::BeginListBox(language.getTranslation("%FILTER_IN_USE%").c_str(), ImVec2(50, 55))) {
                            if (selectable(language.getTranslation("%YES%").c_str(), inUse == YesNoAny::YES, currentID)) { inUse = YesNoAny::YES; }
                            if (inUse == YesNoAny::YES) { ImGui::SetItemDefaultFocus(); }
                            if (selectable(language.getTranslation("%NO%").c_str(), inUse == YesNoAny::NO, currentID)) { inUse = YesNoAny::NO; }
                            if (inUse == YesNoAny::NO) { ImGui::SetItemDefaultFocus(); }
                            if (selectable(language.getTranslation("%ALL%").c_str(), inUse == YesNoAny::ANY, currentID)) { inUse = YesNoAny::ANY; }
                            if (inUse == YesNoAny::ANY) { ImGui::SetItemDefaultFocus(); }
                            ImGui::EndListBox();
                        }

                        // Location
                        ImGui::TableNextColumn();
                        for (int i = 0; i < locations.size(); i++) { checkBox(locations[i], locationFilter.at(i), currentID); }

                        // Cost place
                        ImGui::TableNextColumn();
                        for (int i = 0; i < costPlaces.size(); i++) { checkBox(costPlaces[i], costplaceFilter.at(i), currentID); }

                        // Checkup
                        ImGui::TableNextColumn();
                        if (ImGui::BeginListBox(language.getTranslation("%FILTER_CHECKUP%").c_str(), ImVec2(50, 55))) {
                            if (selectable(language.getTranslation("%YES%").c_str(), requiresCheckup == YesNoAny::YES, currentID)) { requiresCheckup = YesNoAny::YES; }
                            if (requiresCheckup == YesNoAny::YES) { ImGui::SetItemDefaultFocus(); }
                            if (selectable(language.getTranslation("%NO%").c_str(), requiresCheckup == YesNoAny::NO, currentID)) { requiresCheckup = YesNoAny::NO; }
                            if (requiresCheckup == YesNoAny::NO) { ImGui::SetItemDefaultFocus(); }
                            if (selectable(language.getTranslation("%ALL%").c_str(), requiresCheckup == YesNoAny::ANY, currentID)) { requiresCheckup = YesNoAny::ANY; }
                            if (requiresCheckup == YesNoAny::ANY) { ImGui::SetItemDefaultFocus(); }
                            ImGui::EndListBox();
                        }
                        
                        ImGui::EndTable();
                    }
                    
                    ImGui::Separator();

                    //////////////////////////
                    // Filtered device list //
                    //////////////////////////

                    // Filtered list from reference database
                    ImGui::Text(language.getTranslation("%FILTER_DEVICES%").c_str());
                    if (ImGui::BeginTable("DevicesFound", 6, tableFlags)) {
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_ID%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_NAME%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_IN_USE%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_LOCATION%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_COSTPLACE%").c_str());
                        ImGui::TableSetupColumn(language.getTranslation("%FILTER_CHECKUP%").c_str());
                        ImGui::TableHeadersRow();

                        // Display all the devices in the reference table
                        displayedDevices = false;
                        for (int i = 0; i < filterMenu.id.size(); i++) {
                            canDisplay = true;

                            // In use
                            if (inUse != YesNoAny::ANY ? (inUse == YesNoAny::YES ? !filterMenu.inUse[i] : filterMenu.inUse[i]) : false) { canDisplay = false; }

                            // Locations
                            if (!locationFilter[0]) {
                                isEnabled = false;
                                for (int j = 1; j < locations.size(); j++) {
                                    if (filterMenu.location[i].compare(locations[j]) == 0) {
                                        if (locationFilter[j]) { isEnabled = true; break; }
                                    }
                                }
                                if (!isEnabled) { canDisplay = false; }
                            }

                            // Costplace
                            if (!costplaceFilter[0]) {
                                isEnabled = false;
                                for (int j = 1; j < costPlaces.size(); j++) {
                                    if (filterMenu.costplace[i].compare(costPlaces[j]) == 0) {
                                        if (costplaceFilter[j]) { isEnabled = true; break; }
                                    }
                                }
                                if (!isEnabled) { canDisplay = false; }
                            }

                            // Checkup
                            if (requiresCheckup != YesNoAny::ANY ? (requiresCheckup == YesNoAny::YES ? filterMenu.nextCheckup[i] > today : filterMenu.nextCheckup[i] <= today) : false) { canDisplay = false; }

                            // Display
                            if (canDisplay) {
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);
                                ImGui::TableNextColumn();
                                if (button(std::to_string(filterMenu.id[i]).c_str(), currentID)) { deviceMenu.loadDevice(filterMenu.id[i], filterMenu); loadAttempt = true; };
                                ImGui::TableNextColumn();
                                ImGui::Text((const char*)filterMenu.name[i].c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text(filterMenu.inUse[i] ? language.getTranslation("%YES%").c_str() : language.getTranslation("%NO%").c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text((const char*)filterMenu.location[i].c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text((const char*)filterMenu.costplace[i].c_str());
                                ImGui::TableNextColumn();
                                if (filterMenu.nextCheckup[i].t > frequencyDateOffset[0]) { // Date.t is supposed to be a variable only used within the Date struct, but since the frequencyDateOffset's are the only other place time_t's are used it was easier to have it public
                                    bool bTmp = filterMenu.nextCheckup[i] <= today;
                                    checkBox(filterMenu.nextCheckup[i].asString(), bTmp, currentID);
                                } else { ImGui::Text(language.getTranslation("%FILTER_NO_CHECKUPS%").c_str()); }
                                displayedDevices = true;
                            }
                        }
                        if (!displayedDevices) {
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);
                            ImGui::TableNextColumn();
                            ImGui::Text("No");
                            ImGui::TableNextColumn();
                            ImGui::Text("devices");
                            ImGui::TableNextColumn();
                            ImGui::Text("fit");
                            ImGui::TableNextColumn();
                            ImGui::Text("the");
                            ImGui::TableNextColumn();
                            ImGui::Text("selected");
                            ImGui::TableNextColumn();
                            ImGui::Text("filters...");
                        }
                        ImGui::EndTable();
                    }

                    // Make states do the right thing
                    screen = CurrentScreen::FILTER;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(language.getTranslation("%DEVICE%").c_str(), nullptr, loadAttempt ? ImGuiTabItemFlags_SetSelected : NULL)) {
                    if (screen != CurrentScreen::DEVICE) { recommendReload = true; loadAttempt = false; }
                    if (recommendReload) {
                        ImGui::Text(language.getTranslation("%RELOAD_MESSAGE%").c_str());
                        if (button(language.getTranslation("%PRESS_RELOAD%").c_str(), currentID)) { recommendReload = false; reloadFilter = true; }
                        ImGui::Separator();
                    }

                    ImGui::Text(language.getTranslation("%SEARCH_HEADER%").c_str());
                    if (button(language.getTranslation("%SEARCH_DEVICE%").c_str(), currentID)) { deviceMenu.loadDevice(filterMenu.id[searchId], filterMenu); loadAttempt = true; }
                    ImGui::SameLine(); intInput(language.getTranslation("%ID%").c_str(), searchId, currentID);
                    ImGui::Separator();
                    
                    if (deviceMenu.isLoaded) {
                        ImGui::Text(((language.getTranslation("%DEVICE_ID_HEADER%") + " ").c_str() + std::to_string(deviceMenu.data.id)).c_str());
                        if (ImGui::BeginTable("Device", 2, tableFlags)) {
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_INFO%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_VALUE%").c_str());
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_INSTRUMENT%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.name.c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_MODEL%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.model.c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_SERIAL_NUMBER%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(std::to_string(deviceMenu.data.serialnumber).c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_SUPPLIER%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.supplier.c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_DATE_PURCHASE%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.purchaseDate.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_DATE_WARRANTY%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.warrantyDate.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_MANUFACTURER%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.manufacturer.c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_DEPARTMENT%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.department.c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_COSTPLACE%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.costplace.c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_COSTPLACE_NAME%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.costplaceName.c_str());

                            ImGui::EndTable();
                        }

                        ImGui::Text(language.getTranslation("%DEVICE_INTERNAL_CHECK%").c_str());
                        if (ImGui::BeginTable("Internal", 2, tableFlags)) {
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_INFO%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_VALUE%").c_str());
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_INTERNAL_CHECK_FREQUENCY%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation(frequencyNotation[deviceMenu.data.internalFrequency]).c_str());
                            if (deviceMenu.data.internalFrequency) {
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_INTERNAL_CHECK_LAST%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.lastInternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_INTERNAL_CHECK_NEXT%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.nextInternalCheck.asString().c_str());
                            }

                            ImGui::EndTable();
                        }

                        ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK%").c_str());
                        if (ImGui::BeginTable("External", 2, tableFlags)) {
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_INFO%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_VALUE%").c_str());
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK_FREQUENCY%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation(frequencyNotation[deviceMenu.data.externalFrequency]).c_str());
                            if (deviceMenu.data.externalFrequency) {
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK_COMPANY%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.externalCompany.c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK_LAST%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.lastExternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK_NEXT%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.nextExternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK_CONTRACT%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.contractDescription.c_str());
                            }
                            ImGui::EndTable();
                        }

                        ImGui::Text(language.getTranslation("%DEVICE_ADMINISTRATION%").c_str());
                        if (ImGui::BeginTable("Administration", 2, tableFlags)) {
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_INFO%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_VALUE%").c_str());
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_USEABILITY_CHECK_FREQUENCY%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation(frequencyNotation[deviceMenu.data.useabilityFrequency]).c_str());

                            ImGui::EndTable();
                        }

                        ImGui::Text(language.getTranslation("%DEVICE_STATUS%").c_str());
                        if (ImGui::BeginTable("Status", 2, tableFlags)) {
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_INFO%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_VALUE%").c_str());
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            bool used = deviceMenu.data.inUse;
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_IN_USE%").c_str());
                            ImGui::TableNextColumn(); checkBox((std::string) "", used, currentID);
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_DATE_SETUP%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.dateOfSetup.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_DATE_DECOMISSION%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.dateOfDecommissioning.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_WATTAGE%").c_str());
                            ImGui::TableNextColumn(); ImGui::Text(std::to_string(deviceMenu.data.wattage).c_str());

                            ImGui::EndTable();
                        }

                        ImGui::Text(language.getTranslation("%DEVICE_LOGS%").c_str());
                        if (ImGui::BeginTable("Logs", 3, tableFlags)) {
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_LOGS_DATE%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_LOGS_LOGGER%").c_str());
                            ImGui::TableSetupColumn(language.getTranslation("%DEVICE_LOGS_LOG%").c_str());
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            for (int i = 0; i < deviceMenu.data.logDate.size(); i++) {
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.logDate[i].asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.logLogger[i].c_str());
                                ImGui::TableNextColumn(); ImGui::TextWrapped(deviceMenu.data.logLog[i].c_str());
                            }

                            ImGui::EndTable();
                        }
                    } else { ImGui::Text(language.getTranslation("%DEVICE_NOT_FOUND%").c_str()); }

                    // Make states do the right thing
                    screen = CurrentScreen::DEVICE;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(language.getTranslation("%ADMIN%").c_str(), nullptr, NULL)) {
                    //if (screen != CurrentScreen::ADMIN) {}
                    if (editDevice.isLoaded && adminScreen != AdminScreen::EDIT) { editDevice.unloadDevice(); }

                    // ImGui::Text("Screen select");
                    if (button(language.getTranslation("%ADMIN_EDIT%").c_str(), currentID)) { adminScreen = AdminScreen::EDIT; adminDevice.unloadDevice(); }
                    ImGui::SameLine(); if (button(language.getTranslation("%ADMIN_ADD%").c_str(), currentID)) { adminScreen = AdminScreen::ADD; adminDevice.unloadDevice(); }
                    ImGui::SameLine(); if (button(language.getTranslation("%ADMIN_DECOMISSION%").c_str(), currentID)) { adminScreen = AdminScreen::DECOMMISSION; adminDevice.unloadDevice(); }
                    ImGui::Separator();
                    switch (adminScreen) {
                    case AdminScreen::EDIT: {
                        ImGui::Text(language.getTranslation("%SEARCH_HEADER%").c_str());
                        if (button(language.getTranslation("%SEARCH_DEVICE%").c_str(), currentID)) { adminDevice.loadDevice(adminSearch, filterMenu); editDevice.loadDevice(adminSearch, filterMenu); }
                        ImGui::SameLine(); intInput(language.getTranslation("%ID%").c_str(), adminSearch, currentID);
                        ImGui::Separator();
                        if (!adminDevice.isLoaded) { ImGui::Text(language.getTranslation("%DEVICE_NOT_FOUND%").c_str()); }
                        else {
                            ImGui::Text((language.getTranslation("%ADMIN_DEVICE_ID_HEADER%") + " " + std::to_string(adminDevice.data.id)).c_str());
                            if (ImGui::BeginTable("Device", 1, tableFlags)) {
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_EDIT%").c_str());
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_INSTRUMENT%").c_str(), editDevice.data.name, currentID);
                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_MODEL%").c_str(), editDevice.data.model, currentID);
                                ImGui::TableNextColumn(); intInput (language.getTranslation("%DEVICE_SERIAL_NUMBER%").c_str(), editDevice.data.serialnumber, currentID);
                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_SUPPLIER%").c_str(), editDevice.data.supplier, currentID);

                                ImGui::TableNextColumn(); dateInput(editDevice.data.purchaseDate, adminEditDevice.purchaseDateString, language.getTranslation("%DEVICE_DATE_PURCHASE%").c_str(), currentID);
                                ImGui::TableNextColumn(); dateInput(editDevice.data.warrantyDate, adminEditDevice.warrantyDateString, language.getTranslation("%DEVICE_DATE_WARRANTY%").c_str(), currentID);

                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_MANUFACTURER%").c_str(), editDevice.data.manufacturer, currentID);
                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_DEPARTMENT%").c_str(), editDevice.data.department, currentID);
                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_COSTPLACE%").c_str(), editDevice.data.costplace, currentID);
                                ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_COSTPLACE_NAME%").c_str(), editDevice.data.costplaceName, currentID);

                                ImGui::EndTable();
                            }

                            ImGui::Text(language.getTranslation("%DEVICE_INTERNAL_CHECK%").c_str()); 
                            if (ImGui::BeginTable("Internal", 1, tableFlags)) {
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_EDIT%").c_str());
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_INTERNAL_CHECK_FREQUENCY%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation(frequencyNotation[editDevice.data.internalFrequency]).c_str());
                                ImGui::SameLine(); if (button("+", currentID)) { editDevice.data.internalFrequency++; editDevice.data.internalFrequency %= 9; }
                                ImGui::SameLine(); if (button("-", currentID)) { editDevice.data.internalFrequency--; while (editDevice.data.internalFrequency < 0) { editDevice.data.internalFrequency += 9; } }
                                if (editDevice.data.internalFrequency) {
                                    ImGui::TableNextColumn(); dateInput(editDevice.data.lastInternalCheck, adminEditDevice.lastInternalCheckString, language.getTranslation("%DEVICE_INTERNAL_CHECK_LAST_HEADER%").c_str(), currentID);
                                    ImGui::TableNextColumn(); ImGui::Text((language.getTranslation("%DEVICE_INTERNAL_CHECK_NEXT_HEADER%").c_str() + (editDevice.data.lastInternalCheck + frequencyDateOffset[editDevice.data.internalFrequency]).asString()).c_str());
                                }

                                ImGui::EndTable();
                            }

                            ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK%").c_str());
                            if (ImGui::BeginTable("External", 1, tableFlags)) {
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_EDIT%").c_str());
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_EXTERNAL_CHECK_FREQUENCY%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation(frequencyNotation[editDevice.data.externalFrequency]).c_str());
                                ImGui::SameLine(); if (button("+", currentID)) { editDevice.data.externalFrequency++; editDevice.data.externalFrequency %= 9; }
                                ImGui::SameLine(); if (button("-", currentID)) { editDevice.data.externalFrequency--; while (editDevice.data.externalFrequency < 0) { editDevice.data.externalFrequency += 9; } }

                                if (editDevice.data.externalFrequency) {
                                    ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_EXTERNAL_CHECK_COMPANY%").c_str(), editDevice.data.externalCompany, currentID);
                                    ImGui::TableNextColumn(); dateInput(editDevice.data.lastExternalCheck, adminEditDevice.lastExternalCheckString, language.getTranslation("%DEVICE_EXTERNAL_CHECK_LAST%").c_str(), currentID);
                                    ImGui::TableNextColumn(); ImGui::Text((language.getTranslation("%DEVICE_EXTERNAL_CHECK_NEXT_HEADER%") + " " + (editDevice.data.lastExternalCheck + frequencyDateOffset[editDevice.data.externalFrequency]).asString()).c_str());
                                    ImGui::TableNextColumn(); textInput(language.getTranslation("%DEVICE_EXTERNAL_CHECK_CONTRACT%").c_str(), editDevice.data.contractDescription, currentID);
                                }

                                ImGui::EndTable();
                            }

                            ImGui::Text(language.getTranslation("%DEVICE_ADMINISTRATION%").c_str());
                            if (ImGui::BeginTable("Administration", 1, tableFlags)) {
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_EDIT%").c_str());
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation("%DEVICE_USEABILITY_CHECK_FREQUENCY%").c_str());
                                ImGui::TableNextColumn(); ImGui::Text(language.getTranslation(frequencyNotation[editDevice.data.useabilityFrequency]).c_str());
                                ImGui::SameLine(); if (button("+", currentID)) { editDevice.data.useabilityFrequency++; editDevice.data.useabilityFrequency %= 9; }
                                ImGui::SameLine(); if (button("-", currentID)) { editDevice.data.useabilityFrequency--; while (editDevice.data.useabilityFrequency < 0) { editDevice.data.useabilityFrequency += 9; } }

                                ImGui::EndTable();
                            }

                            ImGui::Text(language.getTranslation("%DEVICE_STATUS%").c_str());
                            if (ImGui::BeginTable("Status", 1, tableFlags)) {
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_EDIT%").c_str());
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); checkBox(language.getTranslation("%DEVICE_IN_USE%").c_str(), editDevice.data.inUse, currentID);
                                ImGui::TableNextColumn(); dateInput(editDevice.data.dateOfSetup, adminEditDevice.dateOfSetupString, language.getTranslation("%DEVICE_DATE_SETUP%").c_str(), currentID);
                                ImGui::TableNextColumn(); dateInput(editDevice.data.dateOfDecommissioning, adminEditDevice.dateOfDecommissioningString, language.getTranslation("%DEVICE_DATE_DECOMISSION%").c_str(), currentID);
                                ImGui::TableNextColumn(); ImGui::InputFloat(language.getTranslation("%DEVICE_WATTAGE%").c_str(), &editDevice.data.wattage);

                                ImGui::EndTable();
                            }

                            ImGui::Text(language.getTranslation("%DEVICE_LOGS%").c_str());
                            if (ImGui::BeginTable("Logs", 3, tableFlags)) {
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_LOGS_DATE%").c_str());
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_LOGS_LOGGER%").c_str());
                                ImGui::TableSetupColumn(language.getTranslation("%DEVICE_LOGS_LOG%").c_str());
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                for (int i = 0; i < editDevice.data.logDate.size(); i++) {
                                    ImGui::TableNextColumn(); ImGui::Text(editDevice.data.logDate[i].asString().c_str());
                                    ImGui::TableNextColumn(); ImGui::Text(editDevice.data.logLogger[i].c_str());
                                    ImGui::TableNextColumn(); ImGui::TextWrapped(editDevice.data.logLog[i].c_str());
                                }

                                ImGui::EndTable();
                            }

                            if (button(language.getTranslation("%DEVICE_SAVE%").c_str(), currentID)) {
                                // Reference & costplace
                                DB::moveDevice(adminDevice, editDevice); // removes from old location and adds to new location (even if same location for ease of programming)

                                // Log
                                int c;
                                int t = adminDevice.data.logDate.size();
                                while ((c = adminDevice.data.logDate.size()) < editDevice.data.logDate.size()) {
                                    DB::addDeviceLog(editDevice.data.id, editDevice.data.logDate[c], editDevice.data.logLogger[c], editDevice.data.logLog[c]); // add the new logs

                                    // add to adminDevice for next for loop so it doesnt have to push changes or have different array sizes
                                    adminDevice.data.logDate.push_back(editDevice.data.logDate[c]);
                                    adminDevice.data.logLogger.push_back(editDevice.data.logLogger[c]);
                                    adminDevice.data.logLog.push_back(editDevice.data.logLog[c]);
                                }
                                for (int q = 0; q < t; q++) { // update the old logs if needed
                                    std::string tmp = "UPDATE `log` SET ";
                                    bool hasAdded = false;
                                    if (adminDevice.data.logDate[q] != editDevice.data.logDate[q]) {
                                        tmp += "`date` = "+ std::to_string(editDevice.data.logDate[q].asInt64());
                                        hasAdded = true;
                                    }
                                    if (adminDevice.data.logLogger[q].compare(editDevice.data.logLogger[q]) != 0) {
                                        if (hasAdded) { tmp += ", "; } else { hasAdded = true; }
                                        tmp += "`logger` = \"" + editDevice.data.logLogger[q]+"\"";
                                    }
                                    if (adminDevice.data.logLog[q].compare(editDevice.data.logLog[q]) != 0) {
                                        if (hasAdded) { tmp += ", "; } else { hasAdded = true; }
                                        tmp += "`log` = \"" + editDevice.data.logLog[q] + "\"";
                                    }
                                    tmp += " WHERE `date` = " + std::to_string(editDevice.data.logDate[q].asInt64()) + ";";
                                    if (hasAdded) { DB::updateLog(editDevice.data.id, tmp.c_str()); }
                                }
                            }
                        }
                        break;
                    } 
                    case AdminScreen::ADD: {
                        ImGui::Text("Adding screen has not been implemented yet");
                        break;
                    }
                    case AdminScreen::DECOMMISSION: {
                        ImGui::Text(language.getTranslation("%SEARCH_HEADER%").c_str());
                        if (button(language.getTranslation("%SEARCH_DEVICE%").c_str(), currentID)) { adminDevice.loadDevice(adminSearch, filterMenu); }
                        ImGui::SameLine(); intInput(language.getTranslation("%ID%").c_str(), adminSearch, currentID);
                        ImGui::Separator();
                        if (!adminDevice.isLoaded) { ImGui::Text(language.getTranslation("%DEVICE_NOT_FOUND%").c_str()); }
                        else {
                            ImGui::Text("Too bad the message below is still true...");
                        }
                        ImGui::Text("Decommissioning screen has not been implemented yet");
                        break;
                    }
                    default: { ImGui::Text(language.getTranslation("%ADMIN_NO_SCREEN%").c_str()); break; } // AdminScreen::NONE
                    }

                    ImGui::Separator();
                    ImGui::Text("Testing buttons");
                    if (button("Add device", currentID)) { addDevice(current); current++; }
                    if (button("Add 100 devices", currentID)) { for (int i = 0; i < 100; i++) { addDevice(current); current++; } }
                    if (button("Add 1000 devices", currentID)) { for (int i = 0; i < 1000; i++) { addDevice(current); current++; } }

                    // Make states do the right thing
                    screen = CurrentScreen::ADMIN;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(language.getTranslation("%LANGUAGE%").c_str(), nullptr, NULL)) {
                    if (button(language.getTranslation("%LANGUAGE_RELOAD%").c_str(), currentID)) { language.loadLanguage(language.getCurrentLanguage()); language.findLanguages(); reloadFilter = true; }
                    ImGui::Separator();
                    ImGui::Text((language.getTranslation("%LANGUAGE_CURRENT_HEADER%") + " ").c_str()); ImGui::SameLine(); ImGui::Text(language.getCurrentLanguage().c_str());
                    ImGui::Separator();
                    ImGui::Text(language.getTranslation("%LANGUAGE_LOAD_HEADER%").c_str());
                    for (int i = 0; i < language.languages.size(); i++) {
                        if (button(language.languages[i].c_str(), currentID)) { language.loadLanguage(language.languages[i]); reloadFilter = true; }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        ///////////////
        /// ABS End ///
        ///////////////

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) { ResetDevice(); }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) { return false; }

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    return !(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0);
}

void CleanupDeviceD3D() {
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL) { IM_ASSERT(0); }
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) { return true; }
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) { return 0; } // Disable ALT application menu
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}