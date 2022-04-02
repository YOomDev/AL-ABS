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

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const char* frequencyNotation[9] = {
    "No checks",
    "Daily check",
    "Weekly check",
    "Monthly check",
    "Quarterly check",
    "Bi-Yearly check",
    "Yearly check",
    "Every 5 years check",
    "Check once every decenium"
};

static const time_t frequencyDateOffset[9]{
    Date::getOffset( 0, 0, 0).t,
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

static inline void dateInput(Date& date, std::string& str, const char* label) {
    Date::filterDateInput(str);
    if (str.size() == 0) { str = date.asString(); } else { date.fromStr(str); }
    ImGui::InputText(label, &str);
};

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
    Language language;
    
    // Window bools
    bool done = false;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
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

    // Device screen
    int searchId = 0;

    // Admin screen
    int adminSearch = 0;
    AdminScreen adminScreen = AdminScreen::NONE;
    DeviceMenu adminDevice;
    DeviceMenu editDevice;
    EditDeviceDates adminEditDevice;
    EditDeviceDates adminAddDevice;

    // Styling the program
    const float DARK_FACTOR = 0.7f;
    // colors
    const ImVec4 GREEN = ImVec4(0.0f, 0.5f, 0.3f, 1.0f);
    const ImVec4 DARK_GREEN = ImVec4(0.0f, 0.5f * DARK_FACTOR, 0.3f * DARK_FACTOR, 1.0f);
    const ImVec4 LIGHT_GREEN = ImVec4(0.0f, 0.5f / DARK_FACTOR, 0.3f / DARK_FACTOR, 1.0f);
    const ImVec4 WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 GRAY = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

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
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.5f * DARK_FACTOR * DARK_FACTOR, 0.3f * DARK_FACTOR * DARK_FACTOR, 1.0f);
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
            locations.push_back("Select All");
            costPlaces.push_back("Select All");
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

            // Set window data
            GetWindowRect(hwnd, &hwndSize);
            windowHeight = hwndSize.bottom - hwndSize.top;
            windowWidth = hwndSize.right - hwndSize.left;

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(windowWidth-15, windowHeight-38)); // change to size of window
            ImGui::Begin("ABS", &done, flags | ImGuiWindowFlags_NoTitleBar);

            if (ImGui::BeginTabBar("TABS", NULL)) {
                if (ImGui::BeginTabItem("Filter", nullptr, NULL)) {
                    ////////////////////
                    // Reload message //
                    ////////////////////

                    if (screen != CurrentScreen::FILTER) { recommendReload = true; }
                    if (recommendReload) {
                        ImGui::Text("Something might have changed during your time in a different tab, we would like to recommend you to reload the database...");
                        if (ImGui::Button("Press this to reload")) { recommendReload = false; reloadFilter = true; }
                        ImGui::Separator();
                    }

                    /////////////
                    // Filters //
                    /////////////

                    if (ImGui::BeginTable("DevicesFilter", 4, tableFlags)) {
                        //ImGui::TableSetupColumn("Device ID");
                        //ImGui::TableSetupColumn("Device name");
                        ImGui::TableSetupColumn("Is in use");
                        ImGui::TableSetupColumn("Device location");
                        ImGui::TableSetupColumn("Device costplace");
                        ImGui::TableSetupColumn("Device checkup");
                        ImGui::TableHeadersRow();

                        // Display all the devices in the reference table
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                        // In use
                        ImGui::TableNextColumn();
                        if (ImGui::BeginListBox("Is in use", ImVec2(50, 55))) {
                            if (ImGui::Selectable("Yes", inUse == YesNoAny::YES)) { inUse = YesNoAny::YES; }
                            if (inUse == YesNoAny::YES) { ImGui::SetItemDefaultFocus(); }
                            if (ImGui::Selectable("No", inUse == YesNoAny::NO)) { inUse = YesNoAny::NO; }
                            if (inUse == YesNoAny::NO) { ImGui::SetItemDefaultFocus(); }
                            if (ImGui::Selectable("All", inUse == YesNoAny::ANY)) { inUse = YesNoAny::ANY; }
                            if (inUse == YesNoAny::ANY) { ImGui::SetItemDefaultFocus(); }
                            ImGui::EndListBox();
                        }

                        // Location
                        ImGui::TableNextColumn();
                        for (int i = 0; i < locations.size(); i++) {
                            ImGui::Checkbox(locations[i].c_str(), (bool*) &locationFilter.at(i));
                        }

                        // Cost place
                        ImGui::TableNextColumn();
                        for (int i = 0; i < costPlaces.size(); i++) {
                            ImGui::Checkbox((costPlaces[i] + " ").c_str(), (bool*)&costplaceFilter.at(i));
                        }

                        // Checkup
                        ImGui::TableNextColumn();
                        if (ImGui::BeginListBox("Requires checkup", ImVec2(50, 55))) {
                            if (ImGui::Selectable("Yes ", requiresCheckup == YesNoAny::YES)) { requiresCheckup = YesNoAny::YES; }
                            if (requiresCheckup == YesNoAny::YES) { ImGui::SetItemDefaultFocus(); }
                            if (ImGui::Selectable("No ", requiresCheckup == YesNoAny::NO)) { requiresCheckup = YesNoAny::NO; }
                            if (requiresCheckup == YesNoAny::NO) { ImGui::SetItemDefaultFocus(); }
                            if (ImGui::Selectable("All ", requiresCheckup == YesNoAny::ANY)) { requiresCheckup = YesNoAny::ANY; }
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
                    ImGui::Text("Devices found:");
                    if (ImGui::BeginTable("DevicesFound", 6, tableFlags)) {
                        ImGui::TableSetupColumn("Device ID");
                        ImGui::TableSetupColumn("Device name");
                        ImGui::TableSetupColumn("Is in use");
                        ImGui::TableSetupColumn("Device location");
                        ImGui::TableSetupColumn("Device costplace");
                        ImGui::TableSetupColumn("Device checkup");
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
                                if (ImGui::Button(std::to_string(filterMenu.id[i]).c_str())) { deviceMenu.loadDevice(filterMenu.id[i], filterMenu); loadAttempt = true; };
                                ImGui::TableNextColumn();
                                ImGui::Text((const char*)filterMenu.name[i].c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text(filterMenu.inUse[i] ? "Yes" : "No");
                                ImGui::TableNextColumn();
                                ImGui::Text((const char*)filterMenu.location[i].c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text((const char*)filterMenu.costplace[i].c_str());
                                ImGui::TableNextColumn();
                                bool bTmp = filterMenu.nextCheckup[i] <= today;
                                ImGui::Checkbox(filterMenu.nextCheckup[i].asString().c_str(), &bTmp);
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
                if (ImGui::BeginTabItem("Device", nullptr, loadAttempt ? ImGuiTabItemFlags_SetSelected : NULL)) {
                    if (screen != CurrentScreen::DEVICE) { recommendReload = true; loadAttempt = false; }
                    if (recommendReload) {
                        ImGui::Text("Something might have changed during your time in a different tab, we would like to recommend you to reload the selected device...");
                        if (ImGui::Button("Press this to reload")) { recommendReload = false; reloadFilter = true; }
                        ImGui::Separator();
                    }

                    ImGui::Text("Search:");
                    if (ImGui::Button("Search for device")) { deviceMenu.loadDevice(filterMenu.id[searchId], filterMenu); loadAttempt = true; }
                    ImGui::SameLine(); ImGui::InputInt("id", &searchId);
                    ImGui::Separator();
                    
                    ImGui::Text("Device");
                    if (deviceMenu.isLoaded) {
                        if (ImGui::BeginTable("Device", 2, tableFlags)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("Instrument");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.name.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Model");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.model.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Serial number");
                            ImGui::TableNextColumn(); ImGui::Text(std::to_string(deviceMenu.data.serialnumber).c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Supplier");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.supplier.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Date of purchase");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.purchaseDate.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text("End of warranty");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.warrantyDate.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Manufacturer");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.manufacturer.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Department");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.department.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Cost place");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.costplace.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Cost place name");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.costplaceName.c_str());

                            ImGui::EndTable();
                        }

                        ImGui::Text("Internal check");
                        if (ImGui::BeginTable("Internal", 2, tableFlags)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("Internal check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(frequencyNotation[deviceMenu.data.internalFrequency]);
                            if (deviceMenu.data.internalFrequency) {
                                ImGui::TableNextColumn(); ImGui::Text("Last internal check");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.lastInternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text("Next internal check");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.nextInternalCheck.asString().c_str());
                            }

                            ImGui::EndTable();
                        }

                        ImGui::Text("External check");
                        if (ImGui::BeginTable("External", 2, tableFlags)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("External check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(frequencyNotation[deviceMenu.data.externalFrequency]);
                            if (deviceMenu.data.externalFrequency) {
                                ImGui::TableNextColumn(); ImGui::Text("External company");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.externalCompany.c_str());
                                ImGui::TableNextColumn(); ImGui::Text("Last external check");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.lastExternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text("Next external check");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.nextExternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text("Contract description");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.contractDescription.c_str());
                            }

                            ImGui::EndTable();
                        }

                        ImGui::Text("Administration");
                        if (ImGui::BeginTable("Administration", 2, tableFlags)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("Useability check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(frequencyNotation[0]); // TODO

                            ImGui::EndTable();
                        }

                        ImGui::Text("Status");
                        if (ImGui::BeginTable("Status", 2, tableFlags)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            bool used = deviceMenu.data.inUse;
                            ImGui::TableNextColumn(); ImGui::Text("In use");
                            ImGui::TableNextColumn(); ImGui::Checkbox("", &used); // TODO
                            ImGui::TableNextColumn(); ImGui::Text("Setup date");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.dateOfSetup.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Decommissioning date");
                            ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.dateOfDecommissioning.asString().c_str());
                            ImGui::TableNextColumn(); ImGui::Text("Wattage");
                            ImGui::TableNextColumn(); ImGui::Text(std::to_string(deviceMenu.data.wattage).c_str());

                            ImGui::EndTable();
                        }

                        ImGui::Text("Logs");
                        if (ImGui::BeginTable("Status", 3, tableFlags)) {
                            ImGui::TableSetupColumn("Date");
                            ImGui::TableSetupColumn("Logger");
                            ImGui::TableSetupColumn("Log");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            for (int i = 0; i < deviceMenu.data.logDate.size(); i++) {
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.logDate[i].asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.logLogger[i].c_str());
                                ImGui::TableNextColumn(); ImGui::TextWrapped(deviceMenu.data.logLog[i].c_str());
                            }

                            ImGui::EndTable();
                        }
                    } else { ImGui::Text("Could not find a device with that ID"); }

                    // Make states do the right thing
                    screen = CurrentScreen::DEVICE;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Admin", nullptr, NULL)) {
                    //if (screen != CurrentScreen::ADMIN) {}
                    if (editDevice.isLoaded && adminScreen != AdminScreen::EDIT) { editDevice.unloadDevice(); }

                    ImGui::Text("Screen select");
                    if (ImGui::Button("Edit")) { adminScreen = AdminScreen::EDIT; adminDevice.unloadDevice(); }
                    ImGui::SameLine(); if (ImGui::Button("Add")) { adminScreen = AdminScreen::ADD; adminDevice.unloadDevice(); }
                    ImGui::SameLine(); if (ImGui::Button("Decommission")) { adminScreen = AdminScreen::DECOMMISSION; adminDevice.unloadDevice(); }
                    ImGui::Separator();
                    switch (adminScreen) {
                    case AdminScreen::EDIT: {
                        ImGui::Text("Search:");
                        if (ImGui::Button("Search for device")) { adminDevice.loadDevice(adminSearch, filterMenu); editDevice.loadDevice(adminSearch, filterMenu); }
                        ImGui::SameLine(); ImGui::InputInt("id", &adminSearch);
                        ImGui::Separator();
                        if (!adminDevice.isLoaded) { ImGui::Text("Device with the given id could not be found."); }
                        else {
                            ImGui::Text((std::string("device id: ") + std::to_string(adminDevice.data.id)).c_str());
                            if (ImGui::BeginTable("Device", 1, tableFlags)) {
                                ImGui::TableSetupColumn("Edit");
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::InputText("Instrument", &editDevice.data.name);
                                ImGui::TableNextColumn(); ImGui::InputText("Model", &editDevice.data.model);
                                ImGui::TableNextColumn(); ImGui::InputInt("Serial number", &editDevice.data.serialnumber);
                                ImGui::TableNextColumn(); ImGui::InputText("Supplier", &editDevice.data.supplier);

                                ImGui::TableNextColumn(); dateInput(editDevice.data.purchaseDate, adminEditDevice.purchaseDateString, "Date of purchase");
                                ImGui::TableNextColumn(); dateInput(editDevice.data.warrantyDate, adminEditDevice.warrantyDateString, "End of warranty");

                                ImGui::TableNextColumn(); ImGui::InputText("Manufacturer", &editDevice.data.manufacturer);
                                ImGui::TableNextColumn(); ImGui::InputText("Department", &editDevice.data.department);
                                ImGui::TableNextColumn(); ImGui::InputText("Cost place", &editDevice.data.costplace);
                                ImGui::TableNextColumn(); ImGui::InputText("Cost place name", &editDevice.data.costplaceName);

                                ImGui::EndTable();
                            }

                            ImGui::Text("Internal check");
                            if (ImGui::BeginTable("Internal", 1, tableFlags)) {
                                ImGui::TableSetupColumn("Edit");
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Text("Internal check frequency");
                                ImGui::TableNextColumn(); ImGui::Text(frequencyNotation[editDevice.data.internalFrequency]);
                                ImGui::SameLine(); if (ImGui::Button("+ internal frequency")) { editDevice.data.internalFrequency++; editDevice.data.internalFrequency %= 9; }
                                ImGui::SameLine(); if (ImGui::Button("- internal frequency")) { editDevice.data.internalFrequency--; while (editDevice.data.internalFrequency < 0) { editDevice.data.internalFrequency += 9; } }
                                if (editDevice.data.internalFrequency) {
                                    ImGui::TableNextColumn(); dateInput(editDevice.data.lastInternalCheck, adminEditDevice.lastInternalCheckString, "Last internal check");
                                    ImGui::TableNextColumn(); ImGui::Text(("Next external check: " + (editDevice.data.lastInternalCheck + frequencyDateOffset[editDevice.data.internalFrequency]).asString()).c_str());
                                }

                                ImGui::EndTable();
                            }

                            ImGui::Text("External check");
                            if (ImGui::BeginTable("External", 1, tableFlags)) {
                                ImGui::TableSetupColumn("Edit");
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Text("External check frequency");
                                ImGui::TableNextColumn(); ImGui::Text(frequencyNotation[editDevice.data.externalFrequency]);
                                ImGui::SameLine(); if (ImGui::Button("+ external frequency")) { editDevice.data.externalFrequency++; editDevice.data.externalFrequency %= 9; }
                                ImGui::SameLine(); if (ImGui::Button("- external frequency")) { editDevice.data.externalFrequency--; while (editDevice.data.externalFrequency < 0) { editDevice.data.externalFrequency += 9; } }

                                if (editDevice.data.externalFrequency) {
                                    ImGui::TableNextColumn(); ImGui::InputText("External company", &editDevice.data.externalCompany);
                                    ImGui::TableNextColumn(); dateInput(editDevice.data.lastExternalCheck, adminEditDevice.lastExternalCheckString, "Last external check");
                                    ImGui::TableNextColumn(); ImGui::Text(("Next external check: " + (editDevice.data.lastExternalCheck + frequencyDateOffset[editDevice.data.externalFrequency]).asString()).c_str());
                                    ImGui::TableNextColumn(); ImGui::InputText("Contract description", &editDevice.data.contractDescription);
                                }

                                ImGui::EndTable();
                            }

                            ImGui::Text("Administration");
                            if (ImGui::BeginTable("Administration", 1, tableFlags)) {
                                ImGui::TableSetupColumn("Edit");
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Text("Useability check frequency");
                                ImGui::TableNextColumn(); ImGui::Text(frequencyNotation[editDevice.data.useabilityFrequency]);
                                ImGui::SameLine(); if (ImGui::Button("+ useability frequency")) { editDevice.data.useabilityFrequency++; editDevice.data.useabilityFrequency %= 9; }
                                ImGui::SameLine(); if (ImGui::Button("- useability frequency")) { editDevice.data.useabilityFrequency--; while (editDevice.data.useabilityFrequency < 0) { editDevice.data.useabilityFrequency += 9; } }

                                ImGui::EndTable();
                            }

                            ImGui::Text("Status");
                            if (ImGui::BeginTable("Status", 1, tableFlags)) {
                                ImGui::TableSetupColumn("Edit");
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                ImGui::TableNextColumn(); ImGui::Checkbox("In use", &editDevice.data.inUse);
                                ImGui::TableNextColumn(); dateInput(editDevice.data.dateOfSetup, adminEditDevice.dateOfSetupString, "Setup date");
                                ImGui::TableNextColumn(); dateInput(editDevice.data.dateOfDecommissioning, adminEditDevice.dateOfDecommissioningString, "Decommissioning date");
                                ImGui::TableNextColumn(); ImGui::InputFloat("Wattage", &editDevice.data.wattage);

                                ImGui::EndTable();
                            }

                            ImGui::Text("Logs");
                            if (ImGui::BeginTable("Status", 3, tableFlags)) {
                                ImGui::TableSetupColumn("Date");
                                ImGui::TableSetupColumn("Logger");
                                ImGui::TableSetupColumn("Log");
                                ImGui::TableHeadersRow();
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                                for (int i = 0; i < editDevice.data.logDate.size(); i++) {
                                    ImGui::TableNextColumn(); ImGui::Text(editDevice.data.logDate[i].asString().c_str());
                                    ImGui::TableNextColumn(); ImGui::Text(editDevice.data.logLogger[i].c_str());
                                    ImGui::TableNextColumn(); ImGui::TextWrapped(editDevice.data.logLog[i].c_str());
                                }

                                ImGui::EndTable();
                            }

                            if (ImGui::Button("Save device")) {
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
                        ImGui::Text("Editing screen has not been implemented yet");
                        break;
                    } 
                    case AdminScreen::ADD: {
                        ImGui::Text("Adding screen has not been implemented yet");
                        break;
                    }
                    case AdminScreen::DECOMMISSION: {
                        ImGui::Text("Search:");
                        if (ImGui::Button("Search for device")) { adminDevice.loadDevice(adminSearch, filterMenu); }
                        ImGui::SameLine(); ImGui::InputInt("id", &adminSearch);
                        ImGui::Separator();
                        if (!adminDevice.isLoaded) { ImGui::Text("Device with the given id could not be found."); }
                        else {
                            ImGui::Text("Too bad the message below is still true...");
                        }
                        ImGui::Text("Decommissioning screen has not been implemented yet");
                        break;
                    }
                    default: { ImGui::Text("No screen selected..."); break; } // AdminScreen::NONE
                    }

                    ImGui::Separator();
                    ImGui::Text("Testing buttons");
                    if (ImGui::Button("Add device")) { addDevice(current); current++; }
                    if (ImGui::Button("Add 100 devices")) { for (int i = 0; i < 100; i++) { addDevice(current); current++; } }
                    if (ImGui::Button("Add 1000 devices")) { for (int i = 0; i < 1000; i++) { addDevice(current); current++; } }

                    // Make states do the right thing
                    screen = CurrentScreen::ADMIN;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Language", nullptr, NULL)) {
                    if (ImGui::Button("Reload language files")) { language.loadLanguage(language.getCurrentLanguage()); language.findLanguages(); }
                    if (ImGui::BeginListBox("Languages")) {
                        for (int i = 0; i < language.languages.size(); i++) {
                            if (ImGui::Selectable(language.languages[i].c_str(), isEqual(language.getCurrentLanguage(), language.languages[i]))) { language.loadLanguage(language.languages[i]); }
                            if (isEqual(language.getCurrentLanguage(), language.languages[i])) { ImGui::SetItemDefaultFocus(); }
                        }
                        ImGui::EndListBox();
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