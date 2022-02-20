#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_dx9.h"
#include "ImGui/backends/imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <string>
#include <vector>

#include "DB.h"

//////////
// TODO //
//////////

/*
* BUG:
* Filters for costplace are bugged: first and 4th (currently 4th is last) checkboxes cannot be unchecked
* 
* Scroll to bottom of devices list is not long enough, make sure this works
* 
* TODO:
* Implement admin adding a device the normal way
* Implement admin removing a device
* 
*/

//////////////
// TODO END //
//////////////

enum YesNoAny {
    YES,
    NO,
    ANY
};

enum CurrentScreen {
    FILTER,
    DEVICE,
    ADMIN,
    SERVICE
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

const char* getFrequencyNotation(int f) {
    return "0";
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
    bool noDeviceFound = false;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    CurrentScreen screen = FILTER;
    int w = 400, h = 400;
    bool login = false;

    // Filters
    YesNoAny inUse = YES;
    std::vector<std::string> locations, costPlaces;
    std::vector<unsigned short> locationFilter, costplaceFilter;
    //static const std::string stringANY = "*";
    //std::string selectedLocation = stringANY, selectedCostPlace = stringANY;
    int windowWidth = 1280, windowHeight = 800;
    bool displayedDevices = false;
    bool isEnabled; // filter internal use only
    bool canDisplay; // filter internal use only
    
    // reload bools
    bool recommendReload = false;
    bool loadAttempt = false;
    bool reloadFilter = true;
    bool shouldAdd = false;

    // Device info
    DeviceData adminInfo {0, "test", true, "VERW", "40116", "device_type", 123, "companySupplier", "companyManufacturer", 0, 0, "50-WetChem", "WetChemSpectro", "Admin", "Replacer", true, false, 0, 0, 0, 0, "ExtCheck", 0, 0, 0, "ContractDesc", 0, 0, 1.0f};

    // TMP device add value
    int current = 0;
    std::string name, loc, cost;

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
            costPlaces.clear();
            locations.clear();
            locations.push_back("Select All");
            costPlaces.push_back("Select All");
            filterMenu.reload();
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
            while (locationFilter.size() < locations.size() + 1) { locationFilter.push_back(1); }
            while (costplaceFilter.size() < costPlaces.size() + 1) { costplaceFilter.push_back(1); }
            reloadFilter = false;
            if (filterMenu.id.size() == 0) {
                for (int i = 0; i < 3000; i++) {
                    name = "Device_" + std::to_string(i);
                    loc = (i % 2 ? "VERW" : "OOST") + std::to_string(i % 5);
                    cost = "401" + std::to_string((i % 30) + 10);

                    // DB::addDevice(i, name, i % 2, loc, cost); // TODO: fix: implement DeviceData passthrough
                }
            }
        }
        
        {
            // Set window data
            GetWindowRect(hwnd, &hwndSize);
            windowHeight = hwndSize.bottom - hwndSize.top;
            windowWidth = hwndSize.right - hwndSize.left;

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(windowWidth/* - 200*/, windowHeight)); // change to size of window
            ImGui::Begin("ABS", &done, flags | ImGuiWindowFlags_NoTitleBar);

            if (ImGui::BeginTabBar("TABS", NULL)) {
                if (ImGui::BeginTabItem("Filter", nullptr, NULL)) {
                    ////////////////////
                    // Reload message //
                    ////////////////////

                    if (screen != FILTER) { recommendReload = true; }
                    if (recommendReload) {
                        ImGui::Text("Something might have changed during your time in a different tab, we would like to recommend you to reload the database...");
                        if (ImGui::Button("Press this to reload")) { recommendReload = false; reloadFilter = true; }
                        ImGui::Separator();
                    }

                    /////////////
                    // Filters //
                    /////////////

                    if (ImGui::BeginTable("DevicesFound", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
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
                            if (ImGui::Selectable("Yes", inUse == YES)) { inUse = YES; }
                            if (inUse == YES) { ImGui::SetItemDefaultFocus(); }
                            if (ImGui::Selectable("No", inUse == NO)) { inUse = NO; }
                            if (inUse == NO) { ImGui::SetItemDefaultFocus(); }
                            if (ImGui::Selectable("All", inUse == ANY)) { inUse = ANY; }
                            if (inUse == ANY) { ImGui::SetItemDefaultFocus(); }
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
                            ImGui::Checkbox(costPlaces[i].c_str(), (bool*) &costplaceFilter.at(i));
                        }

                        // checkup
                        ImGui::TableNextColumn();
                        ImGui::Text("TO BE IMPLEMENTED");
                        // TODO
                        
                        
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

                            // in use
                            if (inUse != ANY || inUse == YES ? !filterMenu.inUse[i] : filterMenu.inUse[i]) { canDisplay = false; }

                            // locations
                            if (!locationFilter[0]) {
                                isEnabled = false;
                                for (int j = 1; j < locations.size(); j++) {
                                    if (filterMenu.location[i].compare(locations[j]) == 0) {
                                        if (locationFilter[j]) { isEnabled = true; break; }
                                    }
                                }
                                if (!isEnabled) { canDisplay = false; }
                            }

                            // costplace
                            if (!costplaceFilter[0]) {
                                isEnabled = false;
                                for (int j = 1; j < costPlaces.size(); j++) {
                                    if (filterMenu.costplace[i].compare(costPlaces[j]) == 0) {
                                        if (costplaceFilter[j]) { isEnabled = true; break; }
                                    }
                                }
                                if (!isEnabled) { canDisplay = false; }
                            }

                            // checkup
                            // TODO

                            // display
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
                                ImGui::Text(std::to_string(filterMenu.nextCheckup[i]).c_str());
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
                    screen = FILTER;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Device", deviceMenu.isLoaded ? nullptr : &deviceMenu.isLoaded, loadAttempt ? ImGuiTabItemFlags_SetSelected : NULL)) {
                    if (screen != DEVICE) { recommendReload = true; loadAttempt = false; }
                    if (recommendReload) {
                        ImGui::Text("Something might have changed during your time in a different tab, we would like to recommend you to reload the selected device...");
                        if (ImGui::Button("Press this to reload")) { recommendReload = false; reloadFilter = true; }
                        ImGui::Separator();
                    }

                    ImGui::Text("Search:");
                    ImGui::Text("Implement DEVICE_SEARCH_BAR here...");
                    ImGui::Separator();
                    
                    ImGui::Text("Device");
                    if (deviceMenu.isLoaded) { // TODO: finalize
                        if (ImGui::BeginTable("Device", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn();
                            ImGui::Text("Instrument");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.name.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Model");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.model.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Serial number");
                            ImGui::TableNextColumn();
                            ImGui::Text(std::to_string(deviceMenu.data.serialnumber).c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Supplier");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.supplier.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Date of purchase");
                            ImGui::TableNextColumn();
                            ImGui::Text("0"); // TODO
                            ImGui::TableNextColumn();
                            ImGui::Text("End of warranty");
                            ImGui::TableNextColumn();
                            ImGui::Text("0"); // TODO
                            ImGui::TableNextColumn();
                            ImGui::Text("Manufacturer");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.manufacturer.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Department");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.department.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Cost place");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.costplace.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("Cost place name");
                            ImGui::TableNextColumn();
                            ImGui::Text(deviceMenu.data.costplaceName.c_str());

                            ImGui::EndTable();
                        }

                        ImGui::Text("Useability check");
                        if (ImGui::BeginTable("Useability", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn();
                            ImGui::Text("Useability check frequency");
                            ImGui::TableNextColumn();
                            ImGui::Text(getFrequencyNotation(deviceMenu.data.useabilityFrequency));

                            ImGui::EndTable();
                        }

                        ImGui::Text("Internal check");
                        if (ImGui::BeginTable("Internal", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn();
                            ImGui::Text("Internal check frequency");
                            ImGui::TableNextColumn();
                            ImGui::Text(getFrequencyNotation(deviceMenu.data.internalFrequency));
                            if (deviceMenu.data.internalFrequency) {
                                ImGui::TableNextColumn();
                                ImGui::Text("Last internal check");
                                ImGui::TableNextColumn();
                                ImGui::Text("0"); // TODO
                                ImGui::TableNextColumn();
                                ImGui::Text("Next internal check");
                                ImGui::TableNextColumn();
                                ImGui::Text("0"); // TODO
                            }

                            ImGui::EndTable();
                        }

                        ImGui::Text("External check");
                        if (ImGui::BeginTable("External", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn();
                            ImGui::Text("External check frequency");
                            ImGui::TableNextColumn();
                            ImGui::Text(getFrequencyNotation(deviceMenu.data.externalFrequency));
                            if (deviceMenu.data.externalFrequency) {
                                ImGui::TableNextColumn();
                                ImGui::Text("External company");
                                ImGui::TableNextColumn();
                                ImGui::Text(deviceMenu.data.externalCompany.c_str());
                                ImGui::TableNextColumn();
                                ImGui::Text("Last external check");
                                ImGui::TableNextColumn();
                                ImGui::Text("0"); // TODO
                                ImGui::TableNextColumn();
                                ImGui::Text("Next external check");
                                ImGui::TableNextColumn();
                                ImGui::Text("0"); // TODO
                                ImGui::TableNextColumn();
                                ImGui::Text("Contract description");
                                ImGui::TableNextColumn();
                                ImGui::Text(deviceMenu.data.contractDescription.c_str());
                            }

                            ImGui::EndTable();
                        }

                        ImGui::Text("Administration");
                        if (ImGui::BeginTable("Administration", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            ImGui::TableNextColumn();
                            ImGui::Text("Useability check frequency");
                            ImGui::TableNextColumn();
                            ImGui::Text("0"); // TODO

                            ImGui::EndTable();
                        }

                        ImGui::Text("Status");
                        if (ImGui::BeginTable("Status", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Info");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableHeadersRow();
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 20);

                            bool used = deviceMenu.data.inUse;
                            ImGui::TableNextColumn();
                            ImGui::Text("In use");
                            ImGui::TableNextColumn();
                            ImGui::Checkbox("", &used); // TODO
                            ImGui::TableNextColumn();
                            ImGui::Text("Setup date");
                            ImGui::TableNextColumn();
                            ImGui::Text("0"); // TODO
                            ImGui::TableNextColumn();
                            ImGui::Text("Decommissioning date");
                            ImGui::TableNextColumn();
                            ImGui::Text("0"); // TODO
                            ImGui::TableNextColumn();
                            ImGui::Text("Wattage");
                            ImGui::TableNextColumn();
                            ImGui::Text(std::to_string(deviceMenu.data.wattage).c_str()); // TODO

                            ImGui::EndTable();
                        }
                    } else { ImGui::Text("Could not find a device with that ID"); }

                    // Make states do the right thing
                    screen = DEVICE;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Admin", nullptr, NULL)) {
                    if (screen != ADMIN) { login = false; }

                    // TMP
                    ImGui::Text("Admin panel not implemented yet");

                    ImGui::Separator();
                    if (ImGui::Button("Add device")) { adminInfo.id = current; DB::addDevice(adminInfo); current++; }
                    if (ImGui::Button("Add 1000 devices")) { for (int i = 0; i < 1000; i++) { adminInfo.id = current; DB::addDevice(adminInfo); current++; } }

                    // TODO: add item adding to this page

                    // Make states do the right thing
                    screen = ADMIN;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Service", nullptr, NULL)) {
                    if (screen != SERVICE) { login = false; }

                    // TMP
                    ImGui::Text("Service panel not implemented yet");

                    // Make states do the right thing
                    screen = SERVICE;
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