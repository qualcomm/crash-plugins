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

#include "logcat/logcat.h"
#include <dirent.h> // for directory operations
#include <sys/stat.h> // for file status
#include "logcat.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

std::unordered_map<uint, std::string> event_tags_map = {
    {42, "answer"},
    {314, "pi"},
    {1003, "auditd"},
    {1004, "chatty"},
    {1005, "tag_def"},
    {1006, "liblog"},
    {2718, "e"},
    {2719, "configuration_changed"},
    {2720, "sync"},
    {2721, "cpu"},
    {2722, "battery_level"},
    {2723, "battery_status"},
    {2724, "power_sleep_requested"},
    {2725, "power_screen_broadcast_send"},
    {2726, "power_screen_broadcast_done"},
    {2727, "power_screen_broadcast_stop"},
    {2728, "power_screen_state"},
    {2729, "power_partial_wake_state"},
    {2730, "battery_discharge"},
    {2731, "power_soft_sleep_requested"},
    {2732, "storaged_disk_stats"},
    {2733, "storaged_emmc_info"},
    {2737, "thermal_changed"},
    {2739, "battery_saver_mode"},
    {2740, "location_controller"},
    {2741, "force_gc"},
    {2742, "tickle"},
    {2747, "contacts_aggregation"},
    {2748, "cache_file_deleted"},
    {2749, "storage_state"},
    {2750, "notification_enqueue"},
    {2751, "notification_cancel"},
    {2752, "notification_cancel_all"},
    {2755, "fstrim_start"},
    {2756, "fstrim_finish"},
    {2802, "watchdog"},
    {2803, "watchdog_proc_pss"},
    {2804, "watchdog_soft_reset"},
    {2805, "watchdog_hard_reset"},
    {2806, "watchdog_pss_stats"},
    {2807, "watchdog_proc_stats"},
    {2808, "watchdog_scheduled_reboot"},
    {2809, "watchdog_meminfo"},
    {2810, "watchdog_vmstat"},
    {2811, "watchdog_requested_reboot"},
    {2820, "backup_data_changed"},
    {2821, "backup_start"},
    {2822, "backup_transport_failure"},
    {2823, "backup_agent_failure"},
    {2824, "backup_package"},
    {2825, "backup_success"},
    {2826, "backup_reset"},
    {2827, "backup_initialize"},
    {2828, "backup_requested"},
    {2829, "backup_quota_exceeded"},
    {2830, "restore_start"},
    {2831, "restore_transport_failure"},
    {2832, "restore_agent_failure"},
    {2833, "restore_package"},
    {2834, "restore_success"},
    {2840, "full_backup_package"},
    {2841, "full_backup_agent_failure"},
    {2842, "full_backup_transport_failure"},
    {2843, "full_backup_success"},
    {2844, "full_restore_package"},
    {2845, "full_backup_quota_exceeded"},
    {2846, "full_backup_cancelled"},
    {2850, "backup_transport_lifecycle"},
    {2851, "backup_transport_connection"},
    {2900, "rescue_note"},
    {2901, "rescue_level"},
    {2902, "rescue_success"},
    {2903, "rescue_failure"},
    {3000, "boot_progress_start"},
    {3010, "boot_progress_system_run"},
    {3011, "system_server_start"},
    {3020, "boot_progress_preload_start"},
    {3030, "boot_progress_preload_end"},
    {3040, "boot_progress_ams_ready"},
    {3050, "boot_progress_enable_screen"},
    {3060, "boot_progress_pms_start"},
    {3070, "boot_progress_pms_system_scan_start"},
    {3080, "boot_progress_pms_data_scan_start"},
    {3090, "boot_progress_pms_scan_end"},
    {3100, "boot_progress_pms_ready"},
    {3110, "unknown_sources_enabled"},
    {3120, "pm_critical_info"},
    {3121, "pm_package_stats"},
    {3130, "pm_snapshot_stats"},
    {3131, "pm_snapshot_rebuild"},
    {4000, "calendar_upgrade_receiver"},
    {4100, "contacts_upgrade_receiver"},
    {8000, "job_deferred_execution"},
    {20003, "dvm_lock_sample"},
    {20004, "art_hidden_api_access"},
    {27390, "battery_saver_stats"},
    {27391, "user_activity_timeout_override"},
    {27392, "battery_saver_setting"},
    {27500, "notification_panel_revealed"},
    {27501, "notification_panel_hidden"},
    {27510, "notification_visibility_changed"},
    {27511, "notification_expansion"},
    {27520, "notification_clicked"},
    {27521, "notification_action_clicked"},
    {27530, "notification_canceled"},
    {27531, "notification_visibility"},
    {27532, "notification_alert"},
    {27533, "notification_autogrouped"},
    {27535, "notification_adjusted"},
    {30001, "wm_finish_activity"},
    {30002, "wm_task_to_front"},
    {30003, "wm_new_intent"},
    {30004, "wm_create_task"},
    {30005, "wm_create_activity"},
    {30006, "wm_restart_activity"},
    {30007, "wm_resume_activity"},
    {30008, "am_anr"},
    {30009, "wm_activity_launch_time"},
    {30010, "am_proc_bound"},
    {30011, "am_proc_died"},
    {30012, "wm_failed_to_pause"},
    {30013, "wm_pause_activity"},
    {30014, "am_proc_start"},
    {30015, "am_proc_bad"},
    {30016, "am_proc_good"},
    {30017, "am_low_memory"},
    {30018, "wm_destroy_activity"},
    {30019, "wm_relaunch_resume_activity"},
    {30020, "wm_relaunch_activity"},
    {30021, "wm_on_paused_called"},
    {30022, "wm_on_resume_called"},
    {30023, "am_kill"},
    {30024, "am_broadcast_discard_filter"},
    {30025, "am_broadcast_discard_app"},
    {30030, "am_create_service"},
    {30031, "am_destroy_service"},
    {30032, "am_process_crashed_too_much"},
    {30033, "am_drop_process"},
    {30034, "am_service_crashed_too_much"},
    {30035, "am_schedule_service_restart"},
    {30036, "am_provider_lost_process"},
    {30037, "am_process_start_timeout"},
    {30039, "am_crash"},
    {30040, "am_wtf"},
    {30041, "am_switch_user"},
    {30043, "wm_set_resumed_activity"},
    {30044, "wm_focused_root_task"},
    {30045, "am_pre_boot"},
    {30046, "am_meminfo"},
    {30047, "am_pss"},
    {30048, "wm_stop_activity"},
    {30049, "wm_on_stop_called"},
    {30050, "am_mem_factor"},
    {30051, "am_user_state_changed"},
    {30052, "am_uid_running"},
    {30053, "am_uid_stopped"},
    {30054, "am_uid_active"},
    {30055, "am_uid_idle"},
    {30056, "am_stop_idle_service"},
    {30057, "wm_on_create_called"},
    {30058, "wm_on_restart_called"},
    {30059, "wm_on_start_called"},
    {30060, "wm_on_destroy_called"},
    {30061, "wm_remove_task"},
    {30062, "wm_on_activity_result_called"},
    {30063, "am_compact"},
    {30064, "wm_on_top_resumed_gained_called"},
    {30065, "wm_on_top_resumed_lost_called"},
    {30066, "wm_add_to_stopping"},
    {30067, "wm_set_keyguard_shown"},
    {30068, "am_freeze"},
    {30069, "am_unfreeze"},
    {30070, "uc_finish_user_unlocking"},
    {30071, "uc_finish_user_unlocked"},
    {30072, "uc_finish_user_unlocked_completed"},
    {30073, "uc_finish_user_stopping"},
    {30074, "uc_finish_user_stopped"},
    {30075, "uc_switch_user"},
    {30076, "uc_start_user_internal"},
    {30077, "uc_unlock_user"},
    {30078, "uc_finish_user_boot"},
    {30079, "uc_dispatch_user_switch"},
    {30080, "uc_continue_user_switch"},
    {30081, "uc_send_user_broadcast"},
    {30082, "ssm_user_starting"},
    {30083, "ssm_user_switching"},
    {30084, "ssm_user_unlocking"},
    {30085, "ssm_user_unlocked"},
    {30086, "ssm_user_stopping"},
    {30087, "ssm_user_stopped"},
    {30088, "ssm_user_completed_event"},
    {30100, "am_foreground_service_start"},
    {30101, "am_foreground_service_denied"},
    {30102, "am_foreground_service_stop"},
    {31000, "wm_no_surface_memory"},
    {31001, "wm_task_created"},
    {31002, "wm_task_moved"},
    {31003, "wm_task_removed"},
    {31007, "wm_boot_animation_done"},
    {32000, "imf_force_reconnect_ime"},
    {33000, "wp_wallpaper_crashed"},
    {33001, "wm_wallpaper_surface"},
    {34000, "device_idle"},
    {34001, "device_idle_step"},
    {34002, "device_idle_wake_from_idle"},
    {34003, "device_idle_on_start"},
    {34004, "device_idle_on_phase"},
    {34005, "device_idle_on_complete"},
    {34006, "device_idle_off_start"},
    {34007, "device_idle_off_phase"},
    {34008, "device_idle_off_complete"},
    {34009, "device_idle_light"},
    {34010, "device_idle_light_step"},
    {35000, "auto_brightness_adj"},
    {36000, "sysui_statusbar_touch"},
    {36001, "sysui_heads_up_status"},
    {36002, "sysui_fullscreen_notification"},
    {36003, "sysui_heads_up_escalation"},
    {36004, "sysui_status_bar_state"},
    {36010, "sysui_panelbar_touch"},
    {36020, "sysui_notificationpanel_touch"},
    {36021, "sysui_lockscreen_gesture"},
    {36030, "sysui_quickpanel_touch"},
    {36040, "sysui_panelholder_touch"},
    {36050, "sysui_searchpanel_touch"},
    {36060, "sysui_recents_connection"},
    {36070, "sysui_latency"},
    {40000, "volume_changed"},
    {40001, "stream_devices_changed"},
    {40100, "camera_gesture_triggered"},
    {50000, "menu_item_selected"},
    {50001, "menu_opened"},
    {50020, "connectivity_state_changed"},
    {50021, "wifi_state_changed"},
    {50022, "wifi_event_handled"},
    {50023, "wifi_supplicant_state_changed"},
    {50080, "ntp_success"},
    {50081, "ntp_failure"},
    {50100, "pdp_bad_dns_address"},
    {50101, "pdp_radio_reset_countdown_triggered"},
    {50102, "pdp_radio_reset"},
    {50103, "pdp_context_reset"},
    {50104, "pdp_reregister_network"},
    {50105, "pdp_setup_fail"},
    {50106, "call_drop"},
    {50107, "data_network_registration_fail"},
    {50108, "data_network_status_on_radio_off"},
    {50109, "pdp_network_drop"},
    {50110, "cdma_data_setup_failed"},
    {50111, "cdma_data_drop"},
    {50112, "gsm_rat_switched"},
    {50113, "gsm_data_state_change"},
    {50114, "gsm_service_state_change"},
    {50115, "cdma_data_state_change"},
    {50116, "cdma_service_state_change"},
    {50117, "bad_ip_address"},
    {50118, "data_stall_recovery_get_data_call_list"},
    {50119, "data_stall_recovery_cleanup"},
    {50120, "data_stall_recovery_reregister"},
    {50121, "data_stall_recovery_radio_restart"},
    {50122, "data_stall_recovery_radio_restart_with_prop"},
    {50123, "gsm_rat_switched_new"},
    {50125, "exp_det_sms_denied_by_user"},
    {50128, "exp_det_sms_sent_by_user"},
    {51100, "netstats_mobile_sample"},
    {51101, "netstats_wifi_sample"},
    {51200, "lockdown_vpn_connecting"},
    {51201, "lockdown_vpn_connected"},
    {51202, "lockdown_vpn_error"},
    {51300, "config_install_failed"},
    {51400, "ifw_intent_matched"},
    {51500, "idle_maintenance_window_start"},
    {51501, "idle_maintenance_window_finish"},
    {51600, "timezone_trigger_check"},
    {51610, "timezone_request_install"},
    {51611, "timezone_install_started"},
    {51612, "timezone_install_complete"},
    {51620, "timezone_request_uninstall"},
    {51621, "timezone_uninstall_started"},
    {51622, "timezone_uninstall_complete"},
    {51630, "timezone_request_nothing"},
    {51631, "timezone_nothing_complete"},
    {51690, "timezone_check_trigger_received"},
    {51691, "timezone_check_read_from_data_app"},
    {51692, "timezone_check_request_uninstall"},
    {51693, "timezone_check_request_install"},
    {51694, "timezone_check_request_nothing"},
    {52000, "db_sample"},
    {52001, "http_stats"},
    {52002, "content_query_sample"},
    {52003, "content_update_sample"},
    {52004, "binder_sample"},
    {53000, "harmful_app_warning_uninstall"},
    {53001, "harmful_app_warning_launch_anyway"},
    {60000, "viewroot_draw"},
    {60001, "viewroot_layout"},
    {60002, "view_build_drawing_cache"},
    {60003, "view_use_drawing_cache"},
    {60100, "sf_frame_dur"},
    {60110, "sf_stop_bootanim"},
    {61000, "audioserver_binder_timeout"},
    {62000, "input_interaction"},
    {62001, "input_focus"},
    {62002, "view_enqueue_input_event"},
    {62003, "input_cancel"},
    {62198, "input_dispatcher"},
    {65537, "exp_det_netlink_failure"},
    {70000, "screen_toggled"},
    {70001, "intexcept_power"},
    {70101, "browser_zoom_level_change"},
    {70102, "browser_double_tap_duration"},
    {70150, "browser_snap_center"},
    {70151, "exp_det_attempt_to_call_object_getclass"},
    {70200, "aggregation"},
    {70201, "aggregation_test"},
    {70220, "gms_unknown"},
    {70301, "phone_ui_enter"},
    {70302, "phone_ui_exit"},
    {70303, "phone_ui_button_click"},
    {70304, "phone_ui_ringer_query_elapsed"},
    {70305, "phone_ui_multiple_query"},
    {75000, "sqlite_mem_alarm_current"},
    {75001, "sqlite_mem_alarm_max"},
    {75002, "sqlite_mem_alarm_alloc_attempt"},
    {75003, "sqlite_mem_released"},
    {75004, "sqlite_db_corrupt"},
    {76001, "tts_speak_success"},
    {76002, "tts_speak_failure"},
    {76003, "tts_v2_speak_success"},
    {76004, "tts_v2_speak_failure"},
    {78001, "exp_det_dispatchCommand_overflow"},
    {80100, "bionic_event_memcpy_buffer_overflow"},
    {80105, "bionic_event_strcat_buffer_overflow"},
    {80110, "bionic_event_memmov_buffer_overflow"},
    {80115, "bionic_event_strncat_buffer_overflow"},
    {80120, "bionic_event_strncpy_buffer_overflow"},
    {80125, "bionic_event_memset_buffer_overflow"},
    {80130, "bionic_event_strcpy_buffer_overflow"},
    {80200, "bionic_event_strcat_integer_overflow"},
    {80205, "bionic_event_strncat_integer_overflow"},
    {80300, "bionic_event_resolver_old_response"},
    {80305, "bionic_event_resolver_wrong_server"},
    {80310, "bionic_event_resolver_wrong_query"},
    {81002, "dropbox_file_copy"},
    {90100, "exp_det_cert_pin_failure"},
    {90200, "lock_screen_type"},
    {90201, "exp_det_device_admin_activated_by_user"},
    {90202, "exp_det_device_admin_declined_by_user"},
    {90203, "exp_det_device_admin_uninstalled_by_user"},
    {90204, "settings_latency"},
    {120000, "dsu_progress_update"},
    {120001, "dsu_install_complete"},
    {120002, "dsu_install_failed"},
    {120003, "dsu_install_insufficient_space"},
    {150000, "car_helper_start"},
    {150001, "car_helper_boot_phase"},
    {150002, "car_helper_user_starting"},
    {150003, "car_helper_user_switching"},
    {150004, "car_helper_user_unlocking"},
    {150005, "car_helper_user_unlocked"},
    {150006, "car_helper_user_stopping"},
    {150007, "car_helper_user_stopped"},
    {150008, "car_helper_svc_connected"},
    {150050, "car_service_init"},
    {150051, "car_service_vhal_reconnected"},
    {150052, "car_service_set_car_service_helper"},
    {150053, "car_service_on_user_lifecycle"},
    {150055, "car_service_create"},
    {150056, "car_service_connected"},
    {150057, "car_service_destroy"},
    {150058, "car_service_vhal_died"},
    {150059, "car_service_init_boot_user"},
    {150060, "car_service_on_user_removed"},
    {150100, "car_user_svc_initial_user_info_req"},
    {150101, "car_user_svc_initial_user_info_resp"},
    {150103, "car_user_svc_set_initial_user"},
    {150104, "car_user_svc_set_lifecycle_listener"},
    {150105, "car_user_svc_reset_lifecycle_listener"},
    {150106, "car_user_svc_switch_user_req"},
    {150107, "car_user_svc_switch_user_resp"},
    {150108, "car_user_svc_post_switch_user_req"},
    {150109, "car_user_svc_get_user_auth_req"},
    {150110, "car_user_svc_get_user_auth_resp"},
    {150111, "car_user_svc_switch_user_ui_req"},
    {150112, "car_user_svc_switch_user_from_hal_req"},
    {150113, "car_user_svc_set_user_auth_req"},
    {150114, "car_user_svc_set_user_auth_resp"},
    {150115, "car_user_svc_create_user_req"},
    {150116, "car_user_svc_create_user_resp"},
    {150117, "car_user_svc_create_user_user_created"},
    {150118, "car_user_svc_create_user_user_removed"},
    {150119, "car_user_svc_remove_user_req"},
    {150120, "car_user_svc_remove_user_resp"},
    {150121, "car_user_svc_notify_app_lifecycle_listener"},
    {150122, "car_user_svc_notify_internal_lifecycle_listener"},
    {150123, "car_user_svc_pre_creation_requested"},
    {150124, "car_user_svc_pre_creation_status"},
    {150125, "car_user_svc_start_user_in_background_req"},
    {150126, "car_user_svc_start_user_in_background_resp"},
    {150127, "car_user_svc_stop_user_req"},
    {150128, "car_user_svc_stop_user_resp"},
    {150129, "car_user_svc_initial_user_info_req_complete"},
    {150130, "car_user_svc_logout_user_req"},
    {150131, "car_user_svc_logout_user_resp"},
    {150140, "car_user_hal_initial_user_info_req"},
    {150141, "car_user_hal_initial_user_info_resp"},
    {150142, "car_user_hal_switch_user_req"},
    {150143, "car_user_hal_switch_user_resp"},
    {150144, "car_user_hal_post_switch_user_req"},
    {150145, "car_user_hal_get_user_auth_req"},
    {150146, "car_user_hal_get_user_auth_resp"},
    {150147, "car_user_hal_legacy_switch_user_req"},
    {150148, "car_user_hal_set_user_auth_req"},
    {150149, "car_user_hal_set_user_auth_resp"},
    {150150, "car_user_hal_oem_switch_user_req"},
    {150151, "car_user_hal_create_user_req"},
    {150152, "car_user_hal_create_user_resp"},
    {150153, "car_user_hal_remove_user_req"},
    {150171, "car_user_mgr_add_listener"},
    {150172, "car_user_mgr_remove_listener"},
    {150173, "car_user_mgr_disconnected"},
    {150174, "car_user_mgr_switch_user_req"},
    {150175, "car_user_mgr_switch_user_resp"},
    {150176, "car_user_mgr_get_user_auth_req"},
    {150177, "car_user_mgr_get_user_auth_resp"},
    {150178, "car_user_mgr_set_user_auth_req"},
    {150179, "car_user_mgr_set_user_auth_resp"},
    {150180, "car_user_mgr_create_user_req"},
    {150181, "car_user_mgr_create_user_resp"},
    {150182, "car_user_mgr_remove_user_req"},
    {150183, "car_user_mgr_remove_user_resp"},
    {150184, "car_user_mgr_notify_lifecycle_listener"},
    {150185, "car_user_mgr_pre_create_user_req"},
    {150186, "car_user_mgr_logout_user_req"},
    {150187, "car_user_mgr_logout_user_resp"},
    {150200, "car_dp_mgr_remove_user_req"},
    {150201, "car_dp_mgr_remove_user_resp"},
    {150202, "car_dp_mgr_create_user_req"},
    {150203, "car_dp_mgr_create_user_resp"},
    {150204, "car_dp_mgr_start_user_in_background_req"},
    {150205, "car_dp_mgr_start_user_in_background_resp"},
    {150206, "car_dp_mgr_stop_user_req"},
    {150207, "car_dp_mgr_stop_user_resp"},
    {150300, "car_pwr_mgr_state_change"},
    {150301, "car_pwr_mgr_garage_mode"},
    {150302, "car_pwr_mgr_pwr_policy_change"},
    {150303, "car_pwr_mgr_state_req"},
    {201001, "system_update"},
    {201002, "system_update_user"},
    {202001, "vending_reconstruct"},
    {202901, "transaction_event"},
    {203001, "sync_details"},
    {203002, "google_http_request"},
    {204001, "gtalkservice"},
    {204002, "gtalk_connection"},
    {204003, "gtalk_conn_close"},
    {204004, "gtalk_heartbeat_reset"},
    {204005, "c2dm"},
    {205001, "setup_server_timeout"},
    {205002, "setup_required_captcha"},
    {205003, "setup_io_error"},
    {205004, "setup_server_error"},
    {205005, "setup_retries_exhausted"},
    {205006, "setup_no_data_network"},
    {205007, "setup_completed"},
    {205008, "gls_account_tried"},
    {205009, "gls_account_saved"},
    {205010, "gls_authenticate"},
    {205011, "google_mail_switch"},
    {206001, "snet"},
    {206003, "exp_det_snet"},
    {208000, "metrics_heartbeat"},
    {210001, "security_adb_shell_interactive"},
    {210002, "security_adb_shell_command"},
    {210003, "security_adb_sync_recv"},
    {210004, "security_adb_sync_send"},
    {210005, "security_app_process_start"},
    {210006, "security_keyguard_dismissed"},
    {210007, "security_keyguard_dismiss_auth_attempt"},
    {210008, "security_keyguard_secured"},
    {210009, "security_os_startup"},
    {210010, "security_os_shutdown"},
    {210011, "security_logging_started"},
    {210012, "security_logging_stopped"},
    {210013, "security_media_mounted"},
    {210014, "security_media_unmounted"},
    {210015, "security_log_buffer_size_critical"},
    {210016, "security_password_expiration_set"},
    {210017, "security_password_complexity_set"},
    {210018, "security_password_history_length_set"},
    {210019, "security_max_screen_lock_timeout_set"},
    {210020, "security_max_password_attempts_set"},
    {210021, "security_keyguard_disabled_features_set"},
    {210022, "security_remote_lock"},
    {210023, "security_wipe_failed"},
    {210024, "security_key_generated"},
    {210025, "security_key_imported"},
    {210026, "security_key_destroyed"},
    {210027, "security_user_restriction_added"},
    {210028, "security_user_restriction_removed"},
    {210029, "security_cert_authority_installed"},
    {210030, "security_cert_authority_removed"},
    {210031, "security_crypto_self_test_completed"},
    {210032, "security_key_integrity_violation"},
    {210033, "security_cert_validation_failure"},
    {210034, "security_camera_policy_set"},
    {210035, "security_password_complexity_required"},
    {210036, "security_password_changed"},
    {210037, "security_wifi_connection"},
    {210038, "security_wifi_disconnection"},
    {210039, "security_bluetooth_connection"},
    {210040, "security_bluetooth_disconnection"},
    {230000, "service_manager_stats"},
    {230001, "service_manager_slow"},
    {275534, "notification_unautogrouped"},
    {300000, "arc_system_event"},
    {524287, "sysui_view_visibility"},
    {524288, "sysui_action"},
    {524290, "sysui_count"},
    {524291, "sysui_histogram"},
    {524292, "sysui_multi_action"},
    {525000, "commit_sys_config_file"},
    {1010000, "bt_hci_timeout"},
    {1010001, "bt_config_source"},
    {1010002, "bt_hci_unknown_type"},
    {1990000, "frame_delayed"},
    {1990001, "unacceptable_frame_delay"},
    {1990100, "memsw_state_lowmem"},
    {1990101, "mmesw_state_lowswap"},
    {1990200, "power_mode_change"},
    {1990300, "mcd_start_complete"},
    {10195355, "killinfo"},
    {1397638484, "snet_event_log"},
    {1937006964, "stats_log"},
};

bool Logcat::is_LE = false;

/**
 * Constructor - Initialize logcat parser
 * @param swap: Shared pointer to swap information
 */
Logcat::Logcat(std::shared_ptr<Swapinfo> swap) : swap_ptr(swap){
    LOGD("Initializing logcat parser\n");
    tc_logd = find_proc("logd");
    if (tc_logd) {
        LOGD("Found logd process: PID=%ld\n", tc_logd->pid);
    } else {
        LOGD("logd process not found\n");
    }
}

/**
 * Destructor - Cleanup resources
 */
Logcat::~Logcat(){
    LOGD("Destroying logcat parser\n");
}

/**
 * Initialize field offsets for logcat structures
 */
void Logcat::init_offset(void) {

}

/**
 * Main command entry point
 */
void Logcat::cmd_main(void) {

}

/**
 * Initialize command help information
 */
void Logcat::init_command(void) {

}

/**
 * Convert log level enum to character representation
 * @param level: Log level enum value
 * @return: Single character representing the log level
 */
std::string Logcat::getLogLevelChar(LogLevel level) {
    switch (level) {
        case LOG_DEFAULT: return "D";  // Default level
        case LOG_VERBOSE: return "V";  // Verbose level
        case LOG_DEBUG: return "D";    // Debug level
        case LOG_INFO: return "I";     // Info level
        case LOG_WARN: return "W";     // Warning level
        case LOG_ERROR: return "E";    // Error level
        case LOG_FATAL: return "F";    // Fatal level
        case LOG_SILENT: return "S";   // Silent level
        default: return "";            // Unknown level
    }
}

/**
 * Parse logcat logs from logd process memory
 * Main entry point for extracting logs from crash dump
 */
void Logcat::parser_logcat_log(){
    LOGD("Starting logcat log parsing\n");
    // Verify logd process exists
    if (!tc_logd){
        LOGE("Can't find logd process!\n");
        return;
    }
    // Check if logs already parsed
    if (!log_list.empty()){
        LOGE("Logs already parsed, count: %zu\n", log_list.size());
        return;
    }
    // Create task context for memory access
    LOGD("Creating task context for logd process\n");
    task_ptr = std::make_shared<UTask>(swap_ptr, tc_logd->task);

    // Parse log buffer address from logd memory
    LOGD("Parsing log buffer address\n");
    ulong logbuf_vaddr = parser_logbuf_addr();

    // Validate the virtual address
    if (!is_uvaddr(logbuf_vaddr, tc_logd)){
        LOGE("Invalid virtual address: %#lx\n", logbuf_vaddr);
        return;
    }
    LOGD("Log buffer address: %#lx\n", logbuf_vaddr);

    /*
     * Note: For Android S and later, the address of LogBuffer (SerializedLogBuffer)
     * is not the same as std::list<SerializedLogChunk> logs_[LOG_ID_MAX] GUARDED_BY(logd_lock);
     * Need to handle version-specific differences
     */
    parser_logbuf(logbuf_vaddr);

    // Cleanup task context
    task_ptr.reset();
    LOGD("Log parsing completed, total logs: %zu\n", log_list.size());
}

/**
 * Remove invalid characters from log message
 * Filters out non-printable characters while preserving newlines
 * @param msg: Input message string
 * @return: Cleaned message string with only valid characters
 */
std::string Logcat::remove_invalid_chars(const std::string& msg) {
    if (msg.empty()) {
        return "";
    }
    // Pre-allocate string to avoid reallocations
    std::string validStr;
    validStr.reserve(msg.size());
    bool hasPrintable = false;

    // Process each character
    for (unsigned char c : msg) {
        if (c == '\n') {
            // Preserve newlines
            validStr += '\n';
        } else if (c >= 0x20 && c <= 0x7E) {
            // Keep printable ASCII characters (space to tilde)
            validStr += c;
            if (c != ' ' && c != '\t') {
                hasPrintable = true;
            }
        } else {
            // Replace invalid characters with space
            validStr += ' ';
        }
    }
    // Return empty string if no printable content
    if (!hasPrintable) {
        return "";
    }
    return validStr;
}


/**
 * Print logcat logs in formatted output
 * @param id: Log ID to filter (ALL for all logs)
 * Format: timestamp pid tid uid level tag message
 */
void Logcat::print_logcat_log(LOG_ID id){
    if (log_list.empty()) {
        LOGE("No logs to print\n");
        return;
    }
    LOGD("Printing logs for ID: %d, total entries: %zu\n", id, log_list.size());
    std::ostringstream oss;
    // Iterate through all log entries
    for (const auto &log_ptr : log_list){
        // Filter by log ID if not ALL
        if(id != ALL && log_ptr->logid != id){
            continue;
        }

        // Clean invalid characters from message
        std::string valid_msg = remove_invalid_chars(log_ptr->msg);
        if(valid_msg.empty()){
             continue;
        }

        // Trim trailing whitespace more efficiently
        auto end = valid_msg.find_last_not_of(" \n\r");
        if (end != std::string::npos) {
            valid_msg.erase(end + 1);
        } else {
            continue; // String contains only whitespace
        }

        // Format log entry: timestamp pid tid uid level tag message
        oss << std::setw(18) << std::left << log_ptr->timestamp << " "
            << std::setw(5) << std::right << log_ptr->pid << " "
            << std::setw(5) << std::right << log_ptr->tid << " "
            << std::setw(6) << std::right << log_ptr->uid << " "
            << getLogLevelChar(log_ptr->priority) << " ";

        // Handle different log types
        if (log_ptr->logid == MAIN || log_ptr->logid == SYSTEM || log_ptr->logid == RADIO
            || log_ptr->logid == CRASH || log_ptr->logid == KERNEL){
            // Regular logs: use tag directly
            oss << log_ptr->tag;
        } else {
            // Event logs: map tag index to event name
            try {
                uint tag_index = std::stoi(log_ptr->tag);
                auto it = event_tags_map.find(tag_index);
                oss << (it != event_tags_map.end() ? it->second : log_ptr->tag);
            } catch (const std::exception&) {
                oss << log_ptr->tag;
            }
        }
        oss << " " << valid_msg << "\n";
    }

    const std::string& output = oss.str();
    if (!output.empty()) {
        PRINT("%s\n", output.c_str());
    }
}


/**
 * Parse event log data and extract event information
 * @param pos: Current position in data buffer
 * @param data: Pointer to event data
 * @param len: Total length of data buffer
 * @return: LogEvent structure containing parsed event data
 */
LogEvent Logcat::get_event(size_t pos, char* data, size_t len) {
    LogEvent event = {-1, "", -1};

    // Validate buffer bounds
    if ((pos + sizeof(int8_t)) >= len) {
        LOGE("Buffer overflow in get_event at pos %zu\n", pos);
        return event;
    }

    // Read event type
    int8_t event_type = *reinterpret_cast<int8_t*>(data);

    switch (event_type) {
        case TYPE_INT: {
            // Parse integer event (32-bit)
            constexpr size_t required_size = sizeof(android_event_int_t);
            if (pos + required_size > len) {
                LOGE("Insufficient data for TYPE_INT\n");
                return event;
            }
            const android_event_int_t& event_int = *reinterpret_cast<const android_event_int_t*>(data);
            event.len = required_size;
            event.type = event_int.type;
            event.val = std::to_string(event_int.data);
            break;
        }
        case TYPE_LONG: {
            // Parse long integer event (64-bit)
            constexpr size_t required_size = sizeof(android_event_long_t);
            if (pos + required_size > len) {
                LOGE("Insufficient data for TYPE_LONG\n");
                return event;
            }
            const android_event_long_t& event_long = *reinterpret_cast<const android_event_long_t*>(data);
            event.len = required_size;
            event.type = event_long.type;
            event.val = std::to_string(event_long.data);
            break;
        }
        case TYPE_FLOAT: {
            // Parse floating point event
            constexpr size_t required_size = sizeof(android_event_float_t);
            if (pos + required_size > len) {
                LOGE("Insufficient data for TYPE_FLOAT\n");
                return event;
            }
            const android_event_float_t& event_float = *reinterpret_cast<const android_event_float_t*>(data);
            event.len = required_size;
            event.type = event_float.type;
            event.val = std::to_string(event_float.data);
            break;
        }
        case TYPE_LIST: {
            // Parse list event (contains multiple elements)
            constexpr size_t required_size = sizeof(android_event_list_t);
            if (pos + required_size > len) {
                LOGE("Insufficient data for TYPE_LIST\n");
                return event;
            }
            const android_event_list_t& event_list = *reinterpret_cast<const android_event_list_t*>(data);
            event.len = required_size;
            event.type = event_list.type;
            event.val = std::to_string(event_list.element_count);
            break;
        }
        case TYPE_STRING: {
            // Parse string event (variable length)
            constexpr size_t header_size = sizeof(android_event_string_t);
            if (pos + header_size > len) {
                LOGE("Insufficient data for TYPE_STRING header\n");
                return event;
            }
            const android_event_string_t& event_str = *reinterpret_cast<const android_event_string_t*>(data);
            const size_t total_size = header_size + event_str.length;
            if (pos + total_size > len) {
                LOGE("Insufficient data for TYPE_STRING content\n");
                return event;
            }
            event.len = total_size;
            event.type = event_str.type;
            event.val.assign(data + header_size, event_str.length);
            break;
        }
        default:
            LOGE("Unknown event type: %d\n", event_type);
            break;
    }
    return event;
}

/**
 * Format timestamp for log display
 * @param tv_sec: Seconds since epoch
 * @param tv_nsec: Nanoseconds component
 * @return: Formatted timestamp string (MM-DD HH:MM:SS.mmm)
 */
std::string Logcat::formatTime(uint32_t tv_sec, long tv_nsec) {
    // Convert to time_t for standard time functions
    std::time_t rtc_time = static_cast<std::time_t>(tv_sec);

    // Calculate milliseconds from nanoseconds
    auto ms = (tv_nsec / 1000000) % 1000;

    // Convert to GMT time structure
    std::tm* tm = std::gmtime(&rtc_time);

    // Format date and time
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%m-%d %H:%M:%S", tm);

    // Append milliseconds
    char result[64];
    snprintf(result, sizeof(result), "%s.%03ld", buffer, ms);

    return std::string(result);
}

/**
 * Search for std::list in process memory
 * Iterates through anonymous VMAs to find std::list structures
 * @param vma_callback: Callback to filter VMA regions
 * @param obj_callback: Callback to validate list objects
 * @return: Address of found std::list, or 0 if not found
 */
size_t Logcat::get_stdlist(const std::function<bool(std::shared_ptr<vma_struct>)>& vma_callback, const std::function<bool(ulong)>& obj_callback) {
    if (!task_ptr) {
        LOGE("Task pointer is null\n");
        return 0;
    }

    LOGD("Searching for std::list in process memory\n");
    int index = 0;

    // Iterate through all anonymous VMA regions
    for (const auto& vma_ptr : task_ptr->for_each_anon_vma()) {
        // Apply VMA filter callback if provided
        if (vma_callback && !vma_callback(vma_ptr)) {
            continue;
        }
        LOGD("Checking VMA[%d]: %#lx-%#lx\n", index, vma_ptr->vm_start, vma_ptr->vm_end);
        // Search for std::list in this VMA
        ulong list_addr = task_ptr->search_stdlist(vma_ptr, vma_ptr->vm_start, obj_callback);
        if (list_addr > 0) {
            LOGD("Found std::list at address: %#lx\n", list_addr);
            return list_addr;
        }
        ++index;
    }

    LOGD("std::list not found after checking %d VMAs\n", index);
    return 0;
}

/**
 * Parse system log entry (MAIN, SYSTEM, RADIO, CRASH, KERNEL)
 * Log format: [priority][tag\0][message]
 *
 * @param log_ptr: Shared pointer to LogEntry to populate
 * @param logbuf: Buffer containing log data
 * @param msg_len: Length of log data
 *
 * Format:
 *   --------------------------------------------------------
 *   |    priority    |          tag         |   log         |
 *   --------------------------------------------------------
 */
void Logcat::parser_system_log(std::shared_ptr<LogEntry> log_ptr, char* logbuf, uint16_t msg_len) {
    // Validate input parameters
    if (!logbuf || msg_len == 0) {
        LOGE("Invalid log buffer or length\n");
        return;
    }

    // Parse priority level (first byte)
    if (logbuf[0] >= LOG_DEFAULT && logbuf[0] <= LOG_SILENT) {
        log_ptr->priority = priorityMap[logbuf[0]];
    } else {
        LOGE("Invalid priority %d, using default\n", logbuf[0]);
        log_ptr->priority = LOG_DEFAULT;
    }

    // Parse tag (null-terminated string after priority)
    const char* tag_start = logbuf + 1;
    const char* tag_end = static_cast<const char*>(memchr(tag_start, '\0', msg_len - 1));
    if (!tag_end) {
        LOGE("Tag not null-terminated\n");
        return;
    }
    log_ptr->tag.assign(tag_start, tag_end - tag_start);

    // Parse message (remaining data after tag)
    const char* msg_start = tag_end + 1;
    size_t msg_length = msg_len - (msg_start - logbuf);
    log_ptr->msg = std::string(msg_start, msg_length);
    LOGD("Parsed system log: tag='%s', priority=%d, msg_len=%zu\n",
                log_ptr->tag.c_str(), log_ptr->priority, msg_length);
}

/**
 * Parse event log entry (EVENTS, STATS, SECURITY)
 * Event logs have a binary format with typed data
 *
 * @param log_ptr: Shared pointer to LogEntry to populate
 * @param logbuf: Buffer containing event log data
 * @param msg_len: Length of event log data
 *
 * Format:
 *  ==============================================================================================================================
 *  |   tagindex   |          EVENT_TYPE_LIST        |   EVENT_TYPE_INT  |   value    | EVENT_TYPE_STRING |    len    |   value  |
 *  ==============================================================================================================================
 *                 |sizeof(uint8_t) + sizeof(uint8_t)| sizeof(uint8_t) + sizeof(value)| sizeof(uint8_t) + sizeof(uint32_t) + len  |
 */
void Logcat::parser_event_log(std::shared_ptr<LogEntry> log_ptr,char* logbuf, uint16_t msg_len){
    // Validate input
    if (!logbuf) {
        LOGE("Null log buffer in parser_event_log\n");
        return;
    }

    LOGD("Parsing event log, length: %u\n", msg_len);

    // Event logs are always INFO level
    log_ptr->priority = LogLevel::LOG_INFO;

    const size_t header_size = sizeof(android_event_header_t);
    size_t pos = 0;
    char* msg_ptr = logbuf;
    std::ostringstream oss;

    // Parse event log entries
    while (pos < msg_len){
        // Check if enough space for header
        if (pos + header_size > msg_len){
            LOGE("Insufficient data for event header\n");
            break;
        }

        // Read event header (contains tag index)
        android_event_header_t head = *reinterpret_cast<android_event_header_t*>(msg_ptr);
        msg_ptr += header_size;
        pos += header_size;

        // Store tag as string representation of index
        log_ptr->tag = std::to_string(head.tag);
        oss << std::left << ":[";

        // Parse first event data
        LogEvent event = get_event(pos, msg_ptr, msg_len);
        if (event.type == -1) {
            LOGE("Failed to parse event data\n");
            break;
        }
        msg_ptr += event.len;
        pos += event.len;

        // Handle list type events (contains multiple elements)
        if (event.type == TYPE_LIST) {
            std::string list_msg;
            int cnt = std::stoi(event.val);
            LOGD("Parsing event list with %d elements\n", cnt);

            // Parse each list element
            for (int i = 0; i < cnt && pos < msg_len; ++i) {
                event = get_event(pos, msg_ptr, msg_len);
                if (!list_msg.empty()) {
                    list_msg += ",";
                }
                list_msg += event.val;
                msg_ptr += event.len;
                pos += event.len;
            }
            oss << list_msg;
        } else {
            // Single value event
            oss << event.val;
        }
        oss << "]" << "\n";
        log_ptr->msg = oss.str();
        oss.str("");
    }
    LOGD("Event log parsed: tag=%s, msg='%s'\n",
                log_ptr->tag.c_str(), log_ptr->msg.c_str());
}

#pragma GCC diagnostic pop
