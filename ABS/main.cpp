#include "ImGui/imgui.h" // ImGui functions
#include "ImGui/backends/imgui_impl_dx9.h" // ImGui backend
#include "ImGui/backends/imgui_impl_win32.h" // ImGui backend
#include <d3d9.h> // Used for graphics device to display Dear ImGui
#include <tchar.h> // ???
#include "DB.h" // My database library

//////////
// TODO //
//////////

/*
* 
* TODO:
* Add DATE support for the database interactions: only need to change database types to BIGINT instead of INT
* Implement admin adding a device the normal way
* Implement admin removing a device
* 
*/

//////////////
// TODO END //
//////////////

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

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const char* getFrequencyNotation(int f) {
    return "0"; // TODO
}

static void addDevice(int id) {
    DeviceData t = { id, "test" + std::to_string(id%5), true, "VERW" + std::to_string(id % 7), std::to_string(id%8 + 40100), "device_type" + std::to_string(id % 7), 123, "companySupplier" + std::to_string(id % 8), "companyManufacturer" + std::to_string(id % 2), 0, 0, std::to_string(id % 15), "WetChemSpectro", "Admin", "Replacer", true, false, 0, 0, 0, 0, "ExtCheck", 0, 0, 0, "ContractDesc", 0, 0, 0.3f * (id % 200) + 20.0f};
    DB::addDevice(t);
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
    bool isEnabled; // filter display
    bool canDisplay; // filter display

    // device screen
    int searchId = 0;

    // admin screen
    AdminScreen adminScreen = AdminScreen::NONE;
    DeviceMenu adminDevice;
    int adminSearch = 0;

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

                    if (ImGui::BeginTable("DevicesFilter", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
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
                    if (ImGui::BeginTable("DevicesFound", 6, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
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
                            ImGui::Text("in");
                            ImGui::TableNextColumn();
                            ImGui::Text("the");
                            ImGui::TableNextColumn();
                            ImGui::Text("list");
                            ImGui::TableNextColumn();
                            ImGui::Text("!");
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
                        if (ImGui::BeginTable("Device", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
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

                        ImGui::Text("Useability check");
                        if (ImGui::BeginTable("Useability", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("Useability check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(getFrequencyNotation(deviceMenu.data.useabilityFrequency));

                            ImGui::EndTable();
                        }

                        ImGui::Text("Internal check");
                        if (ImGui::BeginTable("Internal", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("Internal check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(getFrequencyNotation(deviceMenu.data.internalFrequency));
                            if (deviceMenu.data.internalFrequency) {
                                ImGui::TableNextColumn(); ImGui::Text("Last internal check");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.lastInternalCheck.asString().c_str());
                                ImGui::TableNextColumn(); ImGui::Text("Next internal check");
                                ImGui::TableNextColumn(); ImGui::Text(deviceMenu.data.nextInternalCheck.asString().c_str());
                            }

                            ImGui::EndTable();
                        }

                        ImGui::Text("External check");
                        if (ImGui::BeginTable("External", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("External check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(getFrequencyNotation(deviceMenu.data.externalFrequency));
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
                        if (ImGui::BeginTable("Administration", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn(); ImGui::Text("Useability check frequency");
                            ImGui::TableNextColumn(); ImGui::Text(getFrequencyNotation(0)); // TODO

                            ImGui::EndTable();
                        }

                        ImGui::Text("Status");
                        if (ImGui::BeginTable("Status", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
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
                        if (ImGui::BeginTable("Status", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
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

                    ImGui::Text("Screen select");
                    if (ImGui::Button("Edit")) { adminScreen = AdminScreen::EDIT; adminDevice.unloadDevice(); }
                    ImGui::SameLine(); if (ImGui::Button("Add")) { adminScreen = AdminScreen::ADD; adminDevice.unloadDevice(); }
                    ImGui::SameLine(); if (ImGui::Button("Decommission")) { adminScreen = AdminScreen::DECOMMISSION; adminDevice.unloadDevice(); }
                    ImGui::Separator();
                    switch (adminScreen) {
                    case AdminScreen::EDIT: {
                        ImGui::Text("Search:");
                        if (ImGui::Button("Search for device")) { adminDevice.loadDevice(adminSearch, filterMenu); }
                        ImGui::SameLine(); ImGui::InputInt("id", &adminSearch);
                        ImGui::Separator();
                        if (!adminDevice.isLoaded) { ImGui::Text("Device with the given id could not be found."); }
                        else {
                            // TODO
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
                            // TODO
                        }
                        ImGui::Text("Decommissioning screen has not been implemented yet");
                        break;
                    }
                    default: { ImGui::Text("No screen selected..."); break; } // AdminScreen::NONE
                    }

                    ImGui::Separator();
                    ImGui::Text("Testing buttons");
                    if (ImGui::Button("Add device")) { addDevice(current); current++; }
                    if (ImGui::Button("Add 1000 devices")) { for (int i = 0; i < 1000; i++) { addDevice(current); current++; } }

                    // Make states do the right thing
                    screen = CurrentScreen::ADMIN;
                    ImGui::EndTabItem();
                }
                /*
                if (ImGui::BeginTabItem("Service", nullptr, NULL)) {
                    //if (screen != CurrentScreen::SERVICE) {}

                    ImGui::Text("Service panel not implemented yet");

                    // Make states do the right thing
                    screen = CurrentScreen::SERVICE;
                    ImGui::EndTabItem();
                }
                */
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