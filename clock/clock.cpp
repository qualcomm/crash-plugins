/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "clock.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

#ifndef BUILD_TARGET_TOGETHER
DEFINE_PLUGIN_COMMAND(Clock)
#endif

/**
 * @brief Main command entry point for Clock analysis
 *
 * Parses command line arguments and dispatches to appropriate handler functions:
 * -c: Display all clock providers with detailed information
 * -t: Show clock tree hierarchy with consumers
 * -e: Show only enabled clocks
 * -d: Show only disabled clocks
 * -p: Show only prepared clocks
 */
void Clock::cmd_main(void) {
    // Check minimum argument count
    if (argcnt < 2) {
        LOGD("Insufficient arguments provided, showing usage\n");
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }

    // Initialize and parse clock providers if not already done
    if (provider_list.empty()) {
        LOGD("Clock provider list is empty, initializing offsets and parsing providers\n");
        init_offset();
        parser_clk_providers();
    } else {
        LOGD("Using cached clock provider list with %zu providers\n", provider_list.size());
    }

    int argerrs = 0;
    int c;

    // Parse command line options
    while ((c = getopt(argcnt, args, "ctedp")) != EOF) {
        switch(c) {
            case 'c':
                LOGD("display all clock providers\n");
                print_clk_providers();
                break;
            case 't':
                LOGD("display clock tree hierarchy\n");
                print_clk_tree();
                break;
            case 'e':
                LOGD("display enabled clocks only\n");
                print_enable_clock();
                break;
            case 'd':
                LOGD("display disabled clocks only\n");
                print_disable_clock();
                break;
            case 'p':
                LOGD("display prepared clocks only\n");
                print_prepare_clock();
                break;
            default:
                LOGD("Unknown option: -%c\n", c);
                argerrs++;
                break;
        }
    }
    // Handle argument errors
    if (argerrs) {
        LOGE("Command line argument errors detected: %d\n", argerrs);
        cmd_usage(pc->curcmd, SYNOPSIS);
        return;
    }
}

/**
 * @brief Initialize kernel structure field offsets
 *
 * Sets up field offsets for all clock-related kernel structures used in clock analysis.
 * These offsets are essential for reading clock data from kernel memory across different
 * kernel versions and configurations.
 */
void Clock::init_offset(void) {
    // Initialize of_clk_provider structure offsets
    field_init(of_clk_provider,link);        // List linkage for provider chain
    field_init(of_clk_provider,node);        // Device tree node reference
    field_init(of_clk_provider,data);        // Provider-specific data pointer
    field_init(of_clk_provider,get);         // Clock getter function pointer
    field_init(of_clk_provider,get_hw);      // Hardware clock getter function pointer
    struct_init(of_clk_provider);            // Complete structure size

    // Initialize device_node structure offsets
    field_init(device_node,name);            // Node name from device tree
    field_init(device_node,full_name);       // Full path name in device tree

    // Initialize Qualcomm-specific clock controller offsets
    field_init(qcom_cc,rclks);               // Array of registered clocks
    field_init(qcom_cc,num_rclks);           // Number of registered clocks

    // Initialize clock regmap structure offsets
    field_init(clk_regmap,hw);               // Hardware clock structure

    // Initialize clock hardware structure offsets
    field_init(clk_hw,core);                 // Core clock structure reference

    // Initialize clock core structure offsets
    field_init(clk_core,name);               // Clock name string
    field_init(clk_core,ops);                // Clock operations structure
    field_init(clk_core,hw);                 // Hardware clock reference
    field_init(clk_core,rate);               // Current clock rate (Hz)
    field_init(clk_core,req_rate);           // Requested clock rate (Hz)
    field_init(clk_core,new_rate);           // New rate during rate change (Hz)
    field_init(clk_core,rpm_enabled);        // RPM enable state flag
    field_init(clk_core,boot_enabled);       // Boot-time enable state flag
    field_init(clk_core,enable_count);       // Enable reference count
    field_init(clk_core,prepare_count);      // Prepare reference count
    field_init(clk_core,clks);               // List of clock consumers

    // Initialize clock consumer structure offsets
    field_init(clk,core);                    // Core clock reference
    field_init(clk,clks_node);               // Node in consumer list
    field_init(clk,dev);                     // Associated device pointer
    field_init(clk,dev_id);                  // Device ID string
    field_init(clk,con_id);                  // Connection ID string

    // Initialize onecell clock data structure offsets
    field_init(clk_onecell_data,clks);       // Array of clock pointers
    field_init(clk_onecell_data,clk_num);    // Number of clocks in array

    // Initialize RPM SMD clock descriptor offsets
    field_init(rpm_smd_clk_desc,clks);       // Array of RPM SMD clocks
    field_init(rpm_smd_clk_desc,num_clks);   // Number of RPM SMD clocks

    // Initialize RPMH clock descriptor offsets
    field_init(clk_rpmh_desc,clks);          // Array of RPMH clocks
    field_init(clk_rpmh_desc,num_clks);      // Number of RPMH clocks

    // Initialize complete structure sizes
    struct_init(clk_core);                   // Clock core structure size
    struct_init(qcom_cc);                    // Qualcomm clock controller size
    struct_init(rpm_smd_clk_desc);           // RPM SMD descriptor size
    struct_init(clk_rpmh_desc);              // RPMH descriptor size
}

void Clock::init_command(void) {
    cmd_name = "ccf";
    help_str_list={
        "ccf",                                /* command name */
        "display Common Clock Framework (CCF) information and analysis",  /* short description */
        "[-c] [-t] [-e] [-d] [-p]\n"
        "  This command analyzes the Linux Common Clock Framework from crash dumps,\n"
        "  providing comprehensive information about clock providers, clock trees,\n"
        "  and clock states including enable/disable and prepare/unprepare status.\n"
        "\n"
        "    -c              display all clock providers with detailed clock information\n"
        "    -t              display clock tree hierarchy showing provider-consumer relationships\n"
        "    -e              display only enabled clocks with their current rates\n"
        "    -d              display only disabled clocks for power analysis\n"
        "    -p              display only prepared clocks ready for operation\n",
        "\n",
        "EXAMPLES",
        "  Display all clock provider info:",
        "    %s> ccf -c",
        "    clk_provider: clock-controller",
        "       clk_core               rate   req_rate   new_rate rpm   boot  en    prep  clk_hw           Name",
        "       ffffff8017a8d600  1010.0MHZ   310.0MHZ  1010.0MHZ 0     1     1     1     ffffffd3034531a8 gpu_cc_pll0",
        "       ffffff8017a8dc00     0.0MHZ     0.0MHZ     0.0MHZ 0     0     0     0     ffffffd3034533f0 gpu_cc_crc_ahb_clk",
        "       ffffff8017a8dd00     0.0MHZ     0.0MHZ     0.0MHZ 0     0     0     0     ffffffd303453488 gpu_cc_cx_apb_clk",
        "       ffffff8017a8d900  1010.0MHZ    19.2MHZ  1010.0MHZ 0     0     0     0     ffffffd303453520 gpu_cc_cx_gfx3d_clk",
        "       ffffff8017a8d500  1010.0MHZ    19.2MHZ  1010.0MHZ 0     0     0     0     ffffffd3034535b8 gpu_cc_cx_gfx3d_slv_clk",
        "       ffffff8017a8df00    19.2MHZ    19.2MHZ     0.0MHZ 0     1     1     1     ffffffd3034532c0 gpu_cc_cx_gmu_clk",
        "\n",
        "  Display all clock provider and clock consumers by tree:",
        "    %s> ccf -t",
        "    clk_provider:clock-controller",
        "       clk_core:ffffff8017a8df00  gpu_cc_cx_gmu_clk                             19.2MHZ",
        "           clk:ffffff801625a000  device:5900000,kgsl-3d0",
        "       clk_core:ffffff8014936a00  gpu_cc_gmu_clk_src                            19.2MHZ",
        "       clk_core:ffffff80157fa700  gpu_cc_gx_cxo_clk                             19.2MHZ",
        "       clk_core:ffffff80157fa400  gpu_cc_gx_gfx3d_clk                           1010MHZ",
        "           clk:ffffff801a5b5f00  device:5900000,kgsl-3d0",
        "           clk:ffffff8022db7580  device:5900000,kgsl-3d0",
        "\n",
        "  Display all enable clock info:",
        "    %s> ccf -e",
        "    =============================================",
        "       Enable Clocks from of_clk_providers list",
        "    =============================================",
        "    clk_core         Name                                          Rate",
        "    ffffff801804ab00 dsi0_phy_pll_out_byteclk                      24.6847MHZ",
        "    ffffff801804aa00 dsi0_phy_pll_out_dsiclk                       8.22824MHZ",
        "    ffffff8017a8d600 gpu_cc_pll0                                   1010MHZ",
        "    ffffff8017a8df00 gpu_cc_cx_gmu_clk                             19.2MHZ",
        "    ffffff8017a8d300 gpu_cc_cxo_aon_clk_src                        19.2MHZ",
        "\n",
        "  Display all disable clock info:",
        "    %s> ccf -d",
        "    =============================================",
        "       Disabled Clocks from of_clk_providers list",
        "    =============================================",
        "    clk_core         Name                                          Rate",
        "    ffffff8015c11400 lpass_audio_hw_vote_clk                       0MHZ",
        "    ffffff8017a8dc00 gpu_cc_crc_ahb_clk                            0MHZ",
        "    ffffff8017a8dd00 gpu_cc_cx_apb_clk                             0MHZ",
        "    ffffff8017a8d900 gpu_cc_cx_gfx3d_clk                           1010MHZ",
        "\n",
        "  Display all prepare clock info:",
        "    %s> ccf -p",
        "    =============================================",
        "       Prepare Clocks from of_clk_providers list",
        "    =============================================",
        "    clk_core         Name                                          Rate",
        "    ffffff801804ab00 dsi0_phy_pll_out_byteclk                      24.6847MHZ",
        "    ffffff801804aa00 dsi0_phy_pll_out_dsiclk                       8.22824MHZ",
        "    ffffff8017a8d600 gpu_cc_pll0                                   1010MHZ",
        "    ffffff8017a8df00 gpu_cc_cx_gmu_clk                             19.2MHZ",
        "\n",
    };
}

/**
 * @brief Default constructor
 *
 * Initializes the Clock plugin with default settings.
 */
Clock::Clock(){

}

/**
 * @brief Parse MSM-specific clock provider (placeholder implementation)
 *
 * This function is a placeholder for parsing MSM-specific clock providers.
 * Currently outputs debug information about the data address.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of provider-specific data
 */
void Clock::parser_clk_of_msm_provider(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing MSM clock provider for %s at data address 0x%lx\n", prov_ptr->name.c_str(), data);
    // TODO: Implement MSM-specific clock provider parsing
}

/**
 * @brief Parse SPMI PMIC clock provider (placeholder implementation)
 *
 * This function is a placeholder for parsing SPMI PMIC clock providers.
 * Currently outputs debug information about the data address.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of provider-specific data
 */
void Clock::parser_clk_spmi_pmic(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing SPMI PMIC clock provider for %s at data address 0x%lx\n", prov_ptr->name.c_str(), data);
    // TODO: Implement SPMI PMIC clock provider parsing
}

/**
 * @brief Parse Virtio clock provider (placeholder implementation)
 *
 * This function is a placeholder for parsing Virtio clock providers.
 * Currently outputs debug information about the data address.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of provider-specific data
 */
void Clock::parser_clk_virtio(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing Virtio clock provider for %s at data address 0x%lx\n", prov_ptr->name.c_str(), data);
    // TODO: Implement Virtio clock provider parsing
}

/**
 * @brief Parse simple clock provider with single clock
 *
 * Extracts clock core information from a simple clock provider that
 * contains a single clock structure.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of clock structure
 */
void Clock::parser_clk_simple(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing simple clock provider for %s at data address 0x%lx\n", prov_ptr->name.c_str(), data);

    // Read clock core address from clock structure
    ulong clk_core = read_ulong(data + field_offset(clk,core),"core");
    if (!is_kvaddr(clk_core)) {
        LOGE("Invalid clock core address 0x%lx for simple clock provider\n", clk_core);
        return;
    }

    LOGD("Found valid clock core at 0x%lx for provider %s\n", clk_core, prov_ptr->name.c_str());
    prov_ptr->core_list.push_back(clk_core);
}

/**
 * @brief Parse simple hardware clock provider with single clock
 *
 * Extracts clock core information from a simple hardware clock provider
 * that contains a single clk_hw structure.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of clk_hw structure
 */
void Clock::parser_clk_hw_simple(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing simple hardware clock provider for %s at data address 0x%lx\n", prov_ptr->name.c_str(), data);

    // Read clock core address from hardware clock structure
    ulong clk_core = read_ulong(data + field_offset(clk_hw,core),"core");
    if (!is_kvaddr(clk_core)) {
        LOGE("Invalid clock core address 0x%lx for simple hw clock provider\n", clk_core);
        return;
    }

    LOGD("Found valid clock core at 0x%lx for hw provider %s\n", clk_core, prov_ptr->name.c_str());
    prov_ptr->core_list.push_back(clk_core);
}

/**
 * @brief Parse RPMH clock provider
 *
 * Extracts clock core information from an RPMH clock provider that
 * contains an array of RPMH clocks managed by the RPM hardware.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of clk_rpmh_desc structure
 */
void Clock::parser_clk_rpmh(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing RPMH clock provider for %s at data address 0x%lx\n", prov_ptr->name.c_str(), data);
    // Check if RPMH descriptor structure is available
    if(struct_size(clk_rpmh_desc) == -1){
        LOGE("RPMH descriptor structure not available, please load clk-rpmh.ko first\n");
        return;
    }

    // Read number of clocks and clock array address
    size_t clk_num = read_ulong(data + field_offset(clk_rpmh_desc,num_clks),"num_clks");
    ulong clks = read_ulong(data + field_offset(clk_rpmh_desc,clks),"clks");

    LOGD("RPMH provider %s has %zu clocks at array address 0x%lx\n", prov_ptr->name.c_str(), clk_num, clks);

    // Iterate through all clocks in the array
    for (size_t i = 0; i < clk_num; i++){
        ulong clk_addr = clks + i * sizeof(void *);
        clk_addr = read_ulong(clk_addr,"clk");
        if (!is_kvaddr(clk_addr)) {
            LOGE("Invalid clock address at index %zu: 0x%lx\n", i, clk_addr);
            continue;
        }

        // Extract clock core from hardware clock structure
        ulong clk_core = read_ulong(clk_addr + field_offset(clk_hw,core),"core");
        if (!is_kvaddr(clk_core)) {
            LOGE("Invalid clock core address at index %zu: 0x%lx\n", i, clk_core);
            continue;
        }
        LOGD("Found valid RPMH clock core at 0x%lx (index %zu)\n", clk_core, i);
        prov_ptr->core_list.push_back(clk_core);
    }
    LOGD("RPMH provider %s parsing completed, found %zu valid clocks\n",
         prov_ptr->name.c_str(), prov_ptr->core_list.size());
}

/**
 * @brief Parse onecell clock provider
 *
 * Extracts clock core information from a onecell clock provider that
 * contains an array of clock pointers. This is commonly used for
 * clock controllers that provide multiple clocks.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of clk_onecell_data structure
 */
void Clock::parser_clk_onecell(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing onecell clock provider for %s at data address 0x%lx\n",
         prov_ptr->name.c_str(), data);

    // Read number of clocks and clock array address
    size_t clk_num = read_ulong(data + field_offset(clk_onecell_data,clk_num),"clk_num");
    ulong clks = read_ulong(data + field_offset(clk_onecell_data,clks),"clks");

    LOGD("Onecell provider %s has %zu clocks at array address 0x%lx\n",
         prov_ptr->name.c_str(), clk_num, clks);

    // Iterate through all clocks in the array
    for (size_t i = 0; i < clk_num; i++){
        ulong clk_addr = clks + i * sizeof(void *);
        clk_addr = read_ulong(clk_addr,"clk");
        if (!is_kvaddr(clk_addr)) {
            LOGE("Invalid clock address at index %zu: 0x%lx\n", i, clk_addr);
            continue;
        }

        // Extract clock core from clock structure
        ulong clk_core = read_ulong(clk_addr + field_offset(clk,core),"core");
        if (!is_kvaddr(clk_core)) {
            LOGE("Invalid clock core address at index %zu: 0x%lx\n", i, clk_core);
            continue;
        }

        LOGD("Found valid onecell clock core at 0x%lx (index %zu)\n", clk_core, i);
        prov_ptr->core_list.push_back(clk_core);
    }

    LOGD("Onecell provider %s parsing completed, found %zu valid clocks\n",
         prov_ptr->name.c_str(), prov_ptr->core_list.size());
}

/**
 * @brief Parse RPM SMD clock provider
 *
 * Extracts clock core information from an RPM SMD clock provider that
 * contains an array of RPM SMD clocks managed by the RPM subsystem.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of rpm_smd_clk_desc structure
 */
void Clock::parser_rpm_smd_clk(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing RPM SMD clock provider for %s at data address 0x%lx\n",
         prov_ptr->name.c_str(), data);

    // Check if RPM SMD descriptor structure is available
    if(struct_size(rpm_smd_clk_desc) == -1){
        LOGE("RPM SMD descriptor structure not available, please load rpm_smd_clk_desc.ko first\n");
        return;
    }

    // Read number of clocks and clock array address
    size_t num_clks = read_ulong(data + field_offset(rpm_smd_clk_desc,num_clks),"num_clks");
    ulong clks = read_ulong(data + field_offset(rpm_smd_clk_desc,clks),"clks");

    LOGD("RPM SMD provider %s has %zu clocks at array address 0x%lx\n",
         prov_ptr->name.c_str(), num_clks, clks);

    // Iterate through all clocks in the array
    for (size_t i = 0; i < num_clks; i++){
        ulong clk_addr = clks + i * sizeof(void *);
        clk_addr = read_ulong(clk_addr,"clk");
        if (!is_kvaddr(clk_addr)) {
            LOGE("Invalid clock address at index %zu: 0x%lx\n", i, clk_addr);
            continue;
        }

        // Extract clock core from hardware clock structure
        ulong clk_core = read_ulong(clk_addr + field_offset(clk_hw,core),"core");
        if (!is_kvaddr(clk_core)) {
            LOGE("Invalid clock core address at index %zu: 0x%lx\n", i, clk_core);
            continue;
        }

        LOGD("Found valid RPM SMD clock core at 0x%lx (index %zu)\n", clk_core, i);
        prov_ptr->core_list.push_back(clk_core);
    }

    LOGD("RPM SMD provider %s parsing completed, found %zu valid clocks\n",
         prov_ptr->name.c_str(), prov_ptr->core_list.size());
}

/**
 * @brief Parse Qualcomm clock controller provider
 *
 * Extracts clock core information from a Qualcomm clock controller that
 * contains an array of registered clocks using the clk_regmap structure.
 *
 * @param prov_ptr Shared pointer to clock provider structure
 * @param data Address of qcom_cc structure
 */
void Clock::parser_clk_qcom_cc(std::shared_ptr<clk_provider> prov_ptr,ulong data){
    LOGD("Parsing Qualcomm clock controller for %s at data address 0x%lx\n",
         prov_ptr->name.c_str(), data);

    // Check if Qualcomm clock controller structure is available
    if(struct_size(qcom_cc) == -1){
        LOGE("Qualcomm clock controller structure not available, please load clk-qcom.ko first\n");
        return;
    }

    // Read number of registered clocks and clock array address
    size_t num_rclks = read_ulong(data + field_offset(qcom_cc,num_rclks),"num_rclks");
    ulong rclks = read_ulong(data + field_offset(qcom_cc,rclks),"rclks");

    LOGD("Qualcomm CC provider %s has %zu registered clocks at array address 0x%lx\n",
         prov_ptr->name.c_str(), num_rclks, rclks);

    // Iterate through all registered clocks in the array
    for (size_t i = 0; i < num_rclks; i++){
        ulong clk_addr = rclks + i * sizeof(void *);
        clk_addr = read_ulong(clk_addr,"clk");
        if (!is_kvaddr(clk_addr)) {
            LOGE("Invalid clock regmap address at index %zu: 0x%lx\n", i, clk_addr);
            continue;
        }

        // Extract clock core from regmap->hw->core chain
        ulong clk_core = read_ulong(clk_addr + field_offset(clk_regmap,hw) + field_offset(clk_hw,core),"clk_core");
        if (!is_kvaddr(clk_core)) {
            LOGE("Invalid clock core address at index %zu: 0x%lx\n", i, clk_core);
            continue;
        }

        LOGD("Found valid Qualcomm CC clock core at 0x%lx (index %zu)\n", clk_core, i);
        prov_ptr->core_list.push_back(clk_core);
    }

    LOGD("Qualcomm CC provider %s parsing completed, found %zu valid clocks\n",
         prov_ptr->name.c_str(), prov_ptr->core_list.size());
}

/**
 * @brief Parse and display clock core information
 *
 * Reads clock core structure from kernel memory and formats the information
 * for display including rates, enable/prepare counts, and clock name.
 *
 * @param addr Address of clk_core structure
 */
void Clock::parser_clk_core(ulong addr){
    LOGD("Parsing clock core at address 0x%lx\n", addr);

    // Read clock name from core structure
    ulong name_addr = read_pointer(addr + field_offset(clk_core,name),"name addr");
    std::string core_name = "";
    if (is_kvaddr(name_addr)){
        core_name = read_cstring(name_addr,64, "name");
        LOGD("Clock core name: %s\n", core_name.c_str());
    } else {
        LOGE("Invalid name address 0x%lx for clock core\n", name_addr);
    }

    // Read clock core structure
    void *core_buf = read_struct(addr,"clk_core");
    if (!core_buf) {
        LOGE("Failed to read clock core structure at address 0x%lx\n", addr);
        return;
    }

    // Extract clock core fields
    ulong hw = ULONG(core_buf + field_offset(clk_core,hw));
    ulong rate = ULONG(core_buf + field_offset(clk_core,rate));
    ulong req_rate = ULONG(core_buf + field_offset(clk_core,req_rate));
    ulong new_rate = ULONG(core_buf + field_offset(clk_core,new_rate));
    bool rpm_enabled = BOOL(core_buf + field_offset(clk_core,rpm_enabled));
    bool boot_enabled = ULONG(core_buf + field_offset(clk_core,boot_enabled));
    int enable_count = UINT(core_buf + field_offset(clk_core,enable_count));
    int prepare_count = UINT(core_buf + field_offset(clk_core,prepare_count));
    LOGD("Clock core %s: rate=%lu Hz, enable_count=%d, prepare_count=%d\n",
         core_name.c_str(), rate, enable_count, prepare_count);
    FREEBUF(core_buf);

    // Format output string
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN)  << std::hex << addr << " "
        << std::right << std::setw(7)   << std::setfill(' ') << std::fixed << std::setprecision(1) << (double)rate/1000000 << "MHZ "
        << std::right << std::setw(7)   << std::setfill(' ') << std::fixed << std::setprecision(1) << (double)req_rate/1000000 << "MHZ "
        << std::right << std::setw(7)   << std::setfill(' ') << std::fixed << std::setprecision(1) << (double)new_rate/1000000 << "MHZ "
        << std::left << std::setw(5)    << std::dec << rpm_enabled << " "
        << std::left << std::setw(5)    << std::dec << boot_enabled << " "
        << std::left << std::setw(5)    << std::dec << enable_count << " "
        << std::left << std::setw(5)    << std::dec << prepare_count << " "
        << std::left << std::setw(VADDR_PRLEN) << std::hex << hw << " "
        << core_name;
    PRINT("   %s \n",oss.str().c_str());
}

/**
 * @brief Display all enabled clocks from clock providers
 *
 * Iterates through all clock providers and displays only the clocks
 * that have a positive enable count, indicating they are currently enabled.
 */
void Clock::print_enable_clock(){
    PRINT("=============================================\n");
    PRINT("  Enable Clocks from of_clk_providers list\n");
    PRINT("=============================================\n");
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN) << "clk_core"  << " "
          << std::left << std::setw(45) << "Name" << " "
          << std::left << "Rate" << "\n";

    for (const auto& provider : provider_list) {
        if (provider->core_list.size() == 0){
            LOGD("Skipping provider %s with no clocks\n", provider->name.c_str());
            continue;
        }
        LOGD("Checking %zu clocks in provider %s for enabled state\n",
             provider->core_list.size(), provider->name.c_str());
        size_t provider_enabled = 0;
        for (const auto& core_addr : provider->core_list) {
            std::string core_name;
            ulong name_addr = read_pointer(core_addr + field_offset(clk_core,name),"name addr");
            if (is_kvaddr(name_addr)){
                core_name = read_cstring(name_addr,64, "name");
            }
            if (core_name.empty()){
                core_name = "";
            }
            ulong rate = read_ulong(core_addr + field_offset(clk_core,rate),"rate");
            int enable_count = read_uint(core_addr + field_offset(clk_core,enable_count),"enable");
            if(enable_count > 0){
                LOGD("Found enabled clock: %s (0x%lx) with enable_count=%d, rate=%lu Hz\n",
                     core_name.c_str(), core_addr, enable_count, rate);

                oss << std::left << std::setw(VADDR_PRLEN) << std::hex << core_addr << " "
                    << std::left << std::setw(45)  << core_name << " "
                    << std::left << std::dec << (double)rate/1000000 << "MHZ" << "\n";

                provider_enabled++;
            }
        }
        LOGD("Provider %s has %zu enabled clocks\n", provider->name.c_str(), provider_enabled);
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Display all disabled clocks from clock providers
 *
 * Iterates through all clock providers and displays only the clocks
 * that have both enable_count and prepare_count equal to zero, indicating
 * they are completely disabled.
 */
void Clock::print_disable_clock(){
    PRINT("=============================================\n");
    PRINT("  Disabled Clocks from of_clk_providers list\n");
    PRINT("=============================================\n");
    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN) << "clk_core"  << " "
          << std::left << std::setw(45) << "Name" << " "
          << std::left << "Rate" << "\n";

    for (const auto& provider : provider_list) {
        if (provider->core_list.size() == 0){
            LOGD("Skipping provider %s with no clocks\n", provider->name.c_str());
            continue;
        }
        LOGD("Checking %zu clocks in provider %s for disabled state\n",
             provider->core_list.size(), provider->name.c_str());
        size_t provider_disabled = 0;
        for (const auto& core_addr : provider->core_list) {
            std::string core_name;
            ulong name_addr = read_pointer(core_addr + field_offset(clk_core,name),"name addr");
            if (is_kvaddr(name_addr)){
                core_name = read_cstring(name_addr,64, "name");
            }
            if (core_name.empty()){
                core_name = "";
            }
            ulong rate = read_ulong(core_addr + field_offset(clk_core,rate),"rate");
            int enable_count = read_uint(core_addr + field_offset(clk_core,enable_count),"enable");
            int prepare_count = read_uint(core_addr + field_offset(clk_core,prepare_count),"prepare");

            if(prepare_count == 0 && enable_count == 0){
                LOGD("Found disabled clock: %s (0x%lx) with enable_count=%d, prepare_count=%d, rate=%lu Hz\n",
                     core_name.c_str(), core_addr, enable_count, prepare_count, rate);

                oss << std::left << std::setw(VADDR_PRLEN) << std::hex << core_addr << " "
                    << std::left << std::setw(45)  << core_name << " "
                    << std::left << std::dec << (double)rate/1000000 << "MHZ" << "\n";
                provider_disabled++;
            }
        }
        LOGD("Provider %s has %zu disabled clocks\n", provider->name.c_str(), provider_disabled);
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Display all prepared clocks from clock providers
 *
 * Iterates through all clock providers and displays only the clocks
 * that have a positive prepare count, indicating they are currently prepared.
 */
void Clock::print_prepare_clock(){
    PRINT("=============================================\n");
    PRINT("  Prepare Clocks from of_clk_providers list\n");
    PRINT("=============================================\n");

    std::ostringstream oss;
    oss << std::left << std::setw(VADDR_PRLEN) << "clk_core"  << " "
          << std::left << std::setw(45) << "Name" << " "
          << std::left << "Rate" << "\n";

    for (const auto& provider : provider_list) {
        if (provider->core_list.size() == 0){
            LOGD("Skipping provider %s with no clocks\n", provider->name.c_str());
            continue;
        }

        LOGD("Checking %zu clocks in provider %s for prepared state\n",
             provider->core_list.size(), provider->name.c_str());

        size_t provider_prepared = 0;

        for (const auto& core_addr : provider->core_list) {
            std::string core_name;
            ulong name_addr = read_pointer(core_addr + field_offset(clk_core,name),"name addr");
            if (is_kvaddr(name_addr)){
                core_name = read_cstring(name_addr,64, "name");
            }
            if (core_name.empty()){
                core_name = "";
            }

            ulong rate = read_ulong(core_addr + field_offset(clk_core,rate),"rate");
            int prepare_count = read_uint(core_addr + field_offset(clk_core,prepare_count),"prepare");

            if(prepare_count > 0){
                LOGD("Found prepared clock: %s (0x%lx) with prepare_count=%d, rate=%lu Hz\n",
                     core_name.c_str(), core_addr, prepare_count, rate);

                oss << std::left << std::setw(VADDR_PRLEN) << std::hex << core_addr << " "
                    << std::left << std::setw(45)  << core_name << " "
                    << std::left << std::dec << (double)rate/1000000 << "MHZ" << "\n";
                provider_prepared++;
            }
        }
        LOGD("Provider %s has %zu prepared clocks\n", provider->name.c_str(), provider_prepared);
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Display clock tree hierarchy with consumers
 *
 * Shows the hierarchical relationship between clock providers, clock cores,
 * and their consumers (devices that use the clocks). This provides a tree
 * view of the entire clock framework structure.
 */
void Clock::print_clk_tree(){
    int offset = field_offset(clk,clks_node);
    std::ostringstream oss;

    for (const auto& provider : provider_list) {
        if (provider->core_list.size() == 0){
            LOGE("Skipping provider %s with no clocks\n", provider->name.c_str());
            continue;
        }

        LOGD("Processing provider %s with %zu clock cores\n",
             provider->name.c_str(), provider->core_list.size());

        oss << std::left << "clk_provider:" << std::hex << provider->addr << "  " << provider->name << "\n";
        for (const auto& core_addr : provider->core_list) {
            std::string core_name;
            ulong name_addr = read_pointer(core_addr + field_offset(clk_core,name),"name addr");
            if (is_kvaddr(name_addr)){
                core_name = read_cstring(name_addr,64, "name");
            }
            if (core_name.empty()){
                core_name = "";
            }
            ulong rate = read_ulong(core_addr + field_offset(clk_core,rate),"rate");

            LOGD("Processing clock core %s (0x%lx) with rate %lu Hz\n",
                 core_name.c_str(), core_addr, rate);

            oss << std::left << "    clk_core:" << std::hex << core_addr << "  "
                << std::left << std::setw(45)  << core_name << " "
                << std::left << std::dec << (double)rate/1000000 << "MHZ" << "\n";

            // Traverse consumer list for this clock core
            ulong hlist_head = core_addr + field_offset(clk_core,clks);
            for (const auto& clk_addr : for_each_hlist(hlist_head, offset)) {
                std::string dev_id;
                ulong dev_id_addr = read_pointer(clk_addr + field_offset(clk,dev_id),"dev_id addr");
                ulong con_id_addr = read_pointer(clk_addr + field_offset(clk,con_id),"con_id addr");

                if(!dev_id_addr && !con_id_addr) {
                    LOGD("Skipping consumer at 0x%lx with no device or connection ID\n", clk_addr);
                    continue;
                }

                if (is_kvaddr(dev_id_addr)){
                    dev_id = read_cstring(dev_id_addr,64, "dev_id");
                }
                if (dev_id.empty()){
                    dev_id = "";
                }
                LOGD("Found consumer: clk=0x%lx, device=%s\n", clk_addr, dev_id.c_str());
                oss << std::left << "         clk:" << std::hex << clk_addr << "  --> device:" << dev_id << "\n";
            }
        }
        oss << "\n\n";
    }
    PRINT("%s \n",oss.str().c_str());
}

/**
 * @brief Display detailed information for all clock providers
 *
 * Shows comprehensive information about each clock provider including
 * all clock cores managed by the provider with their detailed status,
 * rates, and configuration information.
 */
void Clock::print_clk_providers(){
    std::ostringstream oss;
    for (const auto& provider : provider_list) {
        if (provider->core_list.size() == 0){
            LOGE("Skipping provider %s with no clocks\n", provider->name.c_str());
            continue;
        }
        PRINT("clk_provider: %s\n",provider->name.c_str());
        // Print table header
        oss << std::left << std::setw(VADDR_PRLEN)   << "clk_core"  << " "
            << std::right << std::setw(10)   << "rate" << " "
            << std::right << std::setw(10)   << "req_rate" << " "
            << std::right << std::setw(10)   << "new_rate" << " "
            << std::left << std::setw(5)    << "rpm" << " "
            << std::left << std::setw(5)    << "boot" << " "
            << std::left << std::setw(5)    << "en" << " "
            << std::left << std::setw(5)    << "prep" << " "
            << std::left << std::setw(VADDR_PRLEN)    << "clk_hw" << " "
            << "Name";
        PRINT("   %s \n",oss.str().c_str());
        oss.str("");
        LOGD("Processing %zu clock cores for provider %s\n",
             provider->core_list.size(), provider->name.c_str());

        // Display each clock core in this provider
        for (const auto& addr : provider->core_list) {
            parser_clk_core(addr);
        }
        PRINT("\n\n");
    }
}

/**
 * @brief Parse all clock providers from kernel's of_clk_providers list
 *
 * Traverses the kernel's global list of clock providers, extracts information
 * about each provider, identifies the provider type based on function pointers,
 * and dispatches to appropriate parser functions to extract clock cores.
 */
void Clock::parser_clk_providers(){
    // Check if of_clk_providers symbol exists in kernel
    if (!csymbol_exists("of_clk_providers")){
        LOGE("of_clk_providers symbol doesn't exist in this kernel!\n");
        return;
    }
    // Get the list head address
    ulong list_head = csymbol_value("of_clk_providers");
    if (!is_kvaddr(list_head)) {
        LOGE("of_clk_providers address 0x%lx is invalid!\n", list_head);
        return;
    }
    size_t provider_count = 0;
    // Traverse the linked list of clock providers
    for (const auto& addr : for_each_list(list_head,field_offset(of_clk_provider, link))) {
        LOGD("Processing clock provider at address 0x%lx\n", addr);

        // Read the provider structure
        void *provider_buf = read_struct(addr,"of_clk_provider");
        if (!provider_buf) {
            LOGE("Failed to read of_clk_provider structure at address 0x%lx\n", addr);
            continue;
        }

        // Create new provider object
        std::shared_ptr<clk_provider> prov_ptr = std::make_shared<clk_provider>();
        prov_ptr->addr = addr;

        // Extract provider structure fields
        ulong node_addr = ULONG(provider_buf + field_offset(of_clk_provider,node));
        ulong get_addr = ULONG(provider_buf + field_offset(of_clk_provider,get));
        ulong get_hw_addr = ULONG(provider_buf + field_offset(of_clk_provider,get_hw));
        ulong data_addr = ULONG(provider_buf + field_offset(of_clk_provider,data));

        LOGD("Provider fields: node=0x%lx, get=0x%lx, get_hw=0x%lx, data=0x%lx\n",
             node_addr, get_addr, get_hw_addr, data_addr);

        FREEBUF(provider_buf);

        // Extract provider name from device tree node
        if (is_kvaddr(node_addr)){
            ulong name_addr = read_pointer(node_addr,"name addr");
            if (is_kvaddr(name_addr)){
                prov_ptr->name = read_cstring(name_addr,64, "name");
                LOGD("Found provider name: %s\n", prov_ptr->name.c_str());
            } else {
                LOGE("Invalid name address 0x%lx for provider\n", name_addr);
            }
        } else {
            LOGE("Invalid node address 0x%lx for provider\n", node_addr);
        }

        provider_list.push_back(prov_ptr);
        provider_count++;

        // Identify provider type and dispatch to appropriate parser
        ulong offset;
        struct syment *sp;
        std::string func_name;

        if (is_kvaddr(get_addr)){
            LOGD("Analyzing get function at address 0x%lx\n", get_addr);
            sp = value_search(get_addr, &offset);
            if (sp){
                func_name = sp->name;
                LOGD("Found get function: %s\n", func_name.c_str());
                if (func_name.find("of_clk_src_simple_get") != std::string::npos) {
                    parser_clk_simple(prov_ptr,data_addr);
                }else if (func_name.find("of_clk_src_onecell_get") != std::string::npos){
                    parser_clk_onecell(prov_ptr,data_addr);
                } else {
                    LOGD("Unknown get function: %s\n", func_name.c_str());
                }
            }else if (get_config_val("CONFIG_COMMON_CLK_MSM") == "y"){
                parser_clk_of_msm_provider(prov_ptr,data_addr);
            } else {
                LOGD("No symbol found for get function at 0x%lx\n", get_addr);
            }
        }else if (is_kvaddr(get_hw_addr)){
            LOGD("Analyzing get_hw function at address 0x%lx\n", get_hw_addr);
            sp = value_search(get_hw_addr, &offset);
            if (sp){
                func_name = sp->name;
                LOGD("Found get_hw function: %s\n", func_name.c_str());
                if (func_name.find("spmi_pmic_div_clk_hw_get") != std::string::npos) {
                    parser_clk_spmi_pmic(prov_ptr,data_addr);
                }else if (func_name.find("qcom_cc_clk_hw_get") != std::string::npos){
                    parser_clk_qcom_cc(prov_ptr,data_addr);
                }else if (func_name.find("of_clk_hw_virtio_get") != std::string::npos){
                    parser_clk_virtio(prov_ptr,data_addr);
                }else if (func_name.find("qcom_smdrpm_clk_hw_get") != std::string::npos){
                    parser_rpm_smd_clk(prov_ptr,data_addr);
                }else if (func_name.find("of_clk_hw_simple_get") != std::string::npos){
                    parser_clk_hw_simple(prov_ptr,data_addr);
                }else if (func_name.find("of_clk_rpmh_hw_get") != std::string::npos){
                    parser_clk_rpmh(prov_ptr,data_addr);
                } else {
                    LOGD("Unknown to handle get_hw function: %s\n", func_name.c_str());
                }
            } else {
                LOGD("No symbol found for get_hw function at 0x%lx\n", get_hw_addr);
            }
        } else {
            LOGD("Provider has no valid get or get_hw function\n");
        }
        LOGD("Completed processing provider %s with %zu clocks\n",
             prov_ptr->name.c_str(), prov_ptr->core_list.size());
    }
    LOGD("Clock provider parsing completed: found %zu providers\n", provider_count);
}

#pragma GCC diagnostic pop
