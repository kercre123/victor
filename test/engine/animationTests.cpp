/**
 * File: animationTests.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/6/18
 *
 * Description: Tests related to animation loading/playback
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#include "engine/components/dataAccessorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"


using namespace Anki;
using namespace Vector;

extern Anki::Vector::CozmoContext* cozmoContext;

// These legacy animations are from the days when animations were exported with float values instead of ints
// The JSON -> binary conversion clamps these values, but the result is that the animations don't match and
// that's totally valid, so don't include these in the load comparison test. No new anims should be added to this list
const std::set<std::string> kLegacyAnimationsWithCastDifferences{
  "anim_speedtap_wait_medium_03",
  "anim_speedtap_loseround_intensity02_02",
  "anim_speedtap_loseround_intensity01_01",
  "anim_speedtap_losehand_02",
  "anim_speedtap_losehand_01",
  "anim_speedtap_losegame_intensity02_02",
  "anim_speedtap_losegame_intensity02_01",
  "anim_speedtap_wingame_intensity02_01",
  "anim_speedtap_lookatplayer",
  "proto_loco_turn180_01",
  "anim_rollblock_fail_01",
  "anim_pokedreaction_01",
  "anim_sdk_no_01",
  "anim_workout_mediumenergy_weak_01",
  "anim_workout_mediumenergy_strong_01",
  "anim_workout_mediumenergy_getout_01",
  "anim_workout_mediumenergy_getin_01",
  "anim_workout_lowenergy_trans_01",
  "anim_workout_lowenergy_strong_01",
  "anim_workout_lowenergy_getout_02",
  "anim_workout_lowenergy_getready_01",
  "anim_workout_lowenergy_getout_01",
  "anim_workout_highenergy_weak_01",
  "anim_workout_highenergy_getready_01",
  "anim_message_tts_getout",
  "anim_message_rewind_01",
  "anim_message_loop_01",
  "anim_message_getout_01",
  "anim_message_getin_01",
  "anim_message_play_reaction_01",
  "anim_workout_mediumenergy_getready_01",
  "anim_vc_refuse_repair_01",
  "anim_vc_refuse_nosparks_01",
  "anim_vc_reaction_yesfaceheardyou_01_head_angle_40",
  "anim_vc_reaction_yesfaceheardyou_01_head_angle_20",
  "anim_vc_reaction_yesfaceheardyou_01",
  "anim_speedtap_wingame_intensity02_02",
  "anim_workout_lowenergy_getin_01",
  "anim_upgrade_reaction_lift_01",
  "anim_timersup_getin_02",
  "anim_speedtap_winround_intensity02_02",
  "anim_timerset_getin_01",
  "anim_timercancel_getin_01",
  "anim_timercancel_01",
  "anim_workout_highenergy_trans_01",
  "anim_sparking_idle_03",
  "anim_sparking_idle_02",
  "anim_message_deleted_short_01",
  "anim_react2block_02",
  "anim_speedtap_winhand_02",
  "anim_rtpmemorymatch_yes_01",
  "anim_rtpmemorymatch_no_01",
  "anim_speedtap_winround_intensity01_01",
  "anim_rtpkeepaway_playerno_02",
  "anim_rtpkeepaway_ideatoplay_01",
  "anim_rtpkeepaway_askforgame_02",
  "anim_rtpkeepaway_askforgame_01",
  "anim_repair_fix_getout_02",
  "anim_repair_mild_reaction_02",
  "anim_repair_severe_fix_getout_02",
  "anim_repair_severe_wakeup_01",
  "anim_repair_severe_idle_03",
  "anim_repair_severe_getin_01",
  "anim_repair_severe_interruption_offcube_01",
  "anim_repair_severe_interruption_edge_01",
  "anim_repair_severe_fix_getout_01",
  "anim_repair_severe_fix_lowerlift_01",
  "anim_rtpmemorymatch_request_01",
  "anim_repair_severe_reaction_02",
  "anim_repair_severe_fix_roundreaction_01",
  "anim_repair_severe_fix_getin_01",
  "anim_repair_severe_fix_head_up_01",
  "anim_repair_severe_fix_head_down_01",
  "anim_repair_severe_fix_lift_up_01",
  "anim_repair_severe_fix_lift_down_01",
  "anim_repair_severe_driving_loop_03",
  "anim_repair_severe_driving_start_01",
  "anim_repair_reaction_01",
  "anim_repair_react_stop_01",
  "anim_repair_react_fall_01",
  "anim_repair_react_hit_01",
  "anim_repair_mild_fix_tread_02",
  "anim_repair_mild_fix_tread_01",
  "anim_repair_mild_fix_lift_02",
  "anim_repair_mild_fix_lift_01",
  "anim_repair_mild_fix_head_02",
  "anim_repair_fix_idle_fullyfull_01",
  "anim_rtpkeepaway_ideatoplay_03",
  "anim_repair_fix_idle_02",
  "anim_repair_severe_fix_fail_01",
  "anim_repair_fix_lowerlift_01",
  "anim_speedtap_look4block_01",
  "anim_repair_fix_getout_01",
  "anim_repair_fix_roundreact_01",
  "anim_repair_fix_head_up_01",
  "anim_timerleft_getin_01",
  "anim_rtpkeepaway_playerno_03",
  "anim_reacttoblock_reacttotopple_01",
  "anim_reacttoblock_focusedeyes_01",
  "anim_pyramid_thankyou_01_head_angle_40",
  "anim_pyramid_thankyou_01",
  "anim_pyramid_reacttocube_happy_high_02",
  "anim_poked_giggle",
  "anim_pyramid_reacttocube_frustrated_high_01",
  "anim_pyramid_reacttocube_frustrated_mid_01",
  "anim_pyramid_reacttocube_frustrated_low_01",
  "anim_pyramid_lookinplaceforfaces_medium_head_angle_40",
  "anim_pyramid_lookinplaceforfaces_medium_head_angle_-20",
  "anim_speedtap_playerno_01",
  "anim_pyramid_lookinplaceforfaces_long_head_angle_40",
  "anim_pyramid_lookinplaceforfaces_medium",
  "anim_pyramid_lookinplaceforfaces_long",
  "anim_rtpkeepaway_playeryes_01",
  "anim_pounce_lookgetout_01",
  "anim_play_driving_loop_03",
  "anim_play_driving_loop_01",
  "anim_peekaboo_fail_01",
  "anim_play_getout_01",
  "anim_play_requestplay_level2_01",
  "anim_play_requestplay_level1_01",
  "anim_pyramid_rt2blocks_01",
  "anim_launch_idle_03",
  "anim_petdetection_shortreaction_01_head_angle_40",
  "anim_speedtap_loseround_intensity02_01",
  "anim_petdetection_misc_01_head_angle_20",
  "anim_petdetection_misc_01_head_angle_-20",
  "anim_petdetection_misc_01",
  "anim_speedtap_player_idle_01",
  "anim_petdetection_cat_02_head_angle_40",
  "anim_petdetection_cat_01_head_angle_20",
  "anim_repair_severe_reaction_01",
  "anim_petdetection_cat_01",
  "anim_rtpkeepaway_playeryes_03",
  "anim_emotionchip_tap_01",
  "anim_peekaboo_idle_03",
  "anim_peekaboo_requestonce_01",
  "anim_launch_search_head_angle_40",
  "anim_peekaboo_requestshorts_01",
  "anim_peekaboo_requesttwice_01",
  "anim_peekaboo_getin_01",
  "anim_petdetection_cat_02_head_angle_-20",
  "anim_play_requestplay_level1_02",
  "anim_metagame_receivedbonusbox_01",
  "anim_metagame_receivedtoken_reaction_01",
  "anim_rtpmemorymatch_request_02",
  "anim_cozmosays_badword_02_head_angle_-20",
  "anim_memorymatch_successhand_player_02",
  "anim_repair_severe_fix_raiselift_01",
  "anim_memorymatch_successhand_cozmo_03",
  "anim_launch_firsttimewakeup_helloplayer",
  "anim_memorymatch_solo_successgame_player_03",
  "anim_memorymatch_solo_successgame_player_01",
  "anim_memorymatch_solo_reacttopattern_01",
  "anim_cozmosings_getout_01",
  "anim_memorymatch_solo_failhand_player_01",
  "anim_launch_eyeson_01",
  "anim_memorymatch_reacttopattern_struggle_03",
  "anim_memorymatch_reacttopattern_02",
  "anim_memorymatch_pointbigleft_01",
  "anim_memorymatch_pointcenter_02",
  "anim_memorymatch_pointsmallleft_fast_01",
  "anim_cozmosings_getin_01",
  "anim_memorymatch_pointcenter_fast_03",
  "anim_petdetection_shortreaction_02_head_angle_-20",
  "anim_vc_reaction_nofaceheardyou_01",
  "anim_memorymatch_pointcenter_fast_02",
  "anim_memorymatch_pointsmallleft_02",
  "anim_memorymatch_idle_02",
  "anim_memorymatch_failhand_player_03",
  "anim_memorymatch_successhand_cozmo_04",
  "anim_repair_fix_lift_up_01",
  "anim_memorymatch_failhand_player_02",
  "anim_memorymatch_failhand_03",
  "anim_memorymatch_failgame_player_02",
  "anim_memorymatch_failgame_player_01",
  "anim_memorymatch_failgame_cozmo_03",
  "anim_speedtap_losegame_intensity03_02",
  "anim_memorymatch_failgame_cozmo_02",
  "anim_speedtap_tap_03",
  "anim_play_idle_03",
  "anim_energy_requestlvltwo_01",
  "anim_petdetection_cat_01_head_angle_40",
  "anim_speedtap_losegame_intensity02_03",
  "anim_greeting_happy_03_head_angle_-20",
  "anim_bouncer_requesttoplay_01",
  "anim_speedtap_wingame_intensity03_01",
  "anim_petdetection_shortreaction_01_head_angle_20",
  "anim_cozmosays_badword_02_head_angle_40",
  "anim_peekaboo_idle_01",
  "anim_turn_to_motion_pounce_right_01",
  "anim_memorymatch_successhand_cozmo_01",
  "anim_memorymatch_getout_01",
  "anim_pyramid_lookinplaceforfaces_long_head_angle_20",
  "anim_peekaboo_success_01",
  "anim_cozmosays_getout_short_01_head_angle_20",
  "anim_cozmosays_getin_short_01_head_angle_40",
  "anim_energy_react_stop_01",
  "anim_guarddog_getout_busted_01",
  "anim_speedtap_ask2play_01",
  "anim_memorymatch_idle_03",
  "anim_launch_search_head_angle_20",
  "anim_pounce_drive_01",
  "anim_memorymatch_pointbigright_fast_01",
  "anim_launch_idle_02",
  "anim_launch_firsttimewakeup_helloplayer_head_angle_-20",
  "anim_launch_firsttimewakeup_helloplayer_head_angle_20",
  "anim_launch_firsttimewakeup_helloplayer_head_angle_40",
  "anim_launch_firsttimewakeup_helloworld",
  "anim_memorymatch_successhand_cozmo_02",
  "anim_gamesetup_reaction_01",
  "anim_energy_requestshortlvl1_01",
  "anim_codelab_bored_01",
  "anim_laser_drive_01",
  "anim_speedtap_losehand_03",
  "anim_speedtap_losegame_intensity03_03",
  "anim_cozmosays_getout_long_01",
  "anim_pyramid_thankyou_01_head_angle_20",
  "anim_bored_event_04",
  "anim_guarddog_interruption_01",
  "anim_greeting_happy_03_head_angle_20",
  "anim_speedtap_idle_01",
  "anim_play_idle_01",
  "anim_energy_requestshortlvl2_01",
  "anim_play_getin_01",
  "anim_pyramid_reacttocube_happy_mid_01",
  "anim_peekaboo_success_02",
  "anim_energy_cubedownlvl2_02",
  "anim_memorymatch_failhand_player_01",
  "anim_petdetection_misc_01_head_angle_40",
  "anim_peekaboo_idle_02",
  "anim_petdetection_cat_02",
  "anim_freeplay_reacttoface_longname_01",
  "anim_repair_severe_fix_getout_03",
  "anim_launch_search_head_angle_-20",
  "anim_memorymatch_solo_failhand_player_03",
  "anim_timerleft_getout_01",
  "anim_guarddog_sleeploop_02",
  "anim_guarddog_sleeploop_01",
  "anim_guarddog_cubedisconnect_01",
  "anim_memorymatch_pointcenter_03",
  "anim_guarddog_getout_untouched_02",
  "anim_guarddog_getout_untouched_01",
  "anim_guarddog_getout_timeout_02",
  "anim_dizzy_pickup_02",
  "anim_guarddog_getout_timeout_01",
  "anim_guarddog_getout_touched_01",
  "anim_upgrade_reaction_tracks_01",
  "anim_guarddog_getout_playersucess_01",
  "anim_guarddog_fakeout_02",
  "anim_petdetection_shortreaction_01_head_angle_-20",
  "anim_timercancel_02",
  "anim_greeting_happy_03",
  "anim_memorymatch_pointbigleft_fast_02",
  "anim_greeting_happy_02",
  "anim_memorymatch_pointsmallright_02",
  "anim_turn_to_motion_pounce_left_01",
  "anim_repair_fix_lift_down_01",
  "anim_peekaboo_requestthrice_01",
  "anim_gif_idk_01",
  "anim_gif_gleeserious_01",
  "anim_gif_no_01",
  "anim_rtpkeepaway_reacttocube_03",
  "anim_pyramid_reacttocube_happy_low_01",
  "anim_emotionchip_tap_02",
  "anim_memorymatch_pointsmallright_fast_02",
  "anim_gamesetup_getin_01_head_angle_20",
  "anim_gif_eyeroll_01",
  "anim_speedtap_tap_02",
  "anim_energy_shortreact_lvl2_01",
  "anim_gamesetup_getin_01_head_angle_-20",
  "anim_petdetection_cat_02_head_angle_20",
  "anim_rtpkeepaway_playeryes_02",
  "anim_memorymatch_pointsmallright_01",
  "anim_gamesetup_idle_02",
  "anim_memorymatch_pointbigright_01",
  "anim_freeplay_reacttoface_wiggle_01",
  "anim_memorymatch_solo_reacttopattern_03",
  "anim_memorymatch_pointbigright_02",
  "anim_dizzy_reaction_soft_01",
  "anim_energy_idlel2_search_02",
  "anim_greeting_happy_03_head_angle_40",
  "anim_memorymatch_pointcenter_fast_01",
  "anim_gamesetup_getin_01",
  "anim_repair_fix_raiselift_01",
  "anim_cozmosays_app_getin",
  "anim_gamesetup_getin_01_head_angle_40",
  "anim_cozmosays_getin_medium_01",
  "anim_workout_highenergy_strong_01",
  "anim_energy_eat_lvl2_05",
  "anim_energy_cubeshake_lv2_02",
  "anim_bouncer_getin_01",
  "anim_petdetection_shortreaction_02_head_angle_40",
  "anim_timerset_getin_02",
  "anim_memorymatch_failgame_cozmo_01",
  "anim_memorymatch_reacttopattern_struggle_02",
  "anim_speedtap_foundblock_01",
  "anim_cozmosays_badword_01_head_angle_20",
  "anim_vc_reaction_nofaceheardyou_getout_01",
  "anim_bouncer_ideatoplay_01",
  "anim_message_tts",
  "anim_rtc_react_01",
  "anim_memorymatch_solo_failhand_player_02",
  "anim_majorwin",
  "anim_energy_idlel1_02",
  "anim_dizzy_reaction_soft_03",
  "anim_dizzy_reaction_medium_01",
  "anim_explorer_getout_01",
  "anim_cozmosays_getin_short_01",
  "anim_cozmosays_getin_short_01_head_angle_20",
  "anim_memorymatch_pointbigleft_02",
  "anim_pyramid_success_01",
  "anim_cozmosays_getout_short_01",
  "anim_energy_requestlvlone_01",
  "anim_energy_cubeshake_01",
  "anim_workout_highenergy_getin_01",
  "anim_energy_reacttocliff_lv2_01",
  "anim_rtpmemorymatch_request_03",
  "anim_repair_fix_idle_01",
  "anim_energy_getout_01",
  "anim_speedtap_tap_01",
  "anim_bouncer_getout_01",
  "anim_vc_refuse_energy_01",
  "anim_repair_mild_fix_fail_01",
  "anim_rtpmemorymatch_yes_02",
  "anim_cozmosings_100_song_01",
  "anim_dizzy_shake_loop_01",
  "anim_codelab_tinyorchestra_takataka_01",
  "anim_vc_laser_lookdown_01",
  "anim_codelab_kitchen_yucky_01",
  "anim_codelab_magicfortuneteller_inquistive",
  "anim_chargerdocking_severerequest_01",
  "anim_rtpkeepaway_reacttocube_01",
  "anim_play_idle_02",
  "anim_petdetection_shortreaction_02_head_angle_20",
  "anim_guarddog_fakeout_01",
  "anim_energy_fail_01",
  "anim_vc_reaction_yesfaceheardyou_01_head_angle_-20",
  "anim_energy_stuckonblock_lv2",
  "anim_repair_fix_getout_03",
  "anim_repair_fix_head_down_01",
  "anim_launch_idle_01",
  "anim_memorymatch_reacttopattern_standard_01",
  "anim_sdk_yes_01",
  "anim_energy_liftoncube_02",
  "anim_energy_idlel2_search_03",
  "anim_energy_idlel2_04",
  "anim_cozmosays_badword_02_head_angle_20",
  "anim_energy_idlel2_02",
  "anim_energy_cubedone_01",
  "anim_repair_severe_idle_01",
  "anim_energy_idlel2_01",
  "anim_petdetection_shortreaction_01",
  "anim_energy_idlel1_01",
  "anim_energy_idlel1_03",
  "anim_energy_failgetout_02",
  "anim_energy_driveloop_03",
  "anim_explorer_getin_01",
  "anim_energy_idlel2_search_01",
  "anim_energy_fail_03",
  "anim_rtpkeepaway_reacttocube_02",
  "anim_energy_drivegetin_01",
  "anim_energy_fail_02",
  "anim_petdetection_shortreaction_02",
  "anim_workout_lowenergy_weak_01",
  "anim_energy_driveloop_01",
  "anim_speedtap_fakeout_01",
  "anim_memorymatch_solo_successgame_player_02",
  "anim_energy_cubedonelvl1_02",
  "anim_energy_eat_03",
  "anim_energy_cubeshakelvl1_02",
  "anim_repair_mild_fix_head_01",
  "anim_energy_cubedown_02",
  "anim_energy_cubedone_02",
  "anim_timerset_getout_01",
  "anim_energy_liftoncube_01",
  "anim_cozmosays_getin_medium_01_head_angle_40",
  "anim_cozmosays_getout_long_01_head_angle_40",
  "anim_energy_eat_lvl1_03",
  "anim_repair_fix_getready_01",
  "anim_energy_cubenotfound_02",
  "anim_energy_cubedownlvl1_03",
  "anim_peekaboo_success_03",
  "anim_emotionchip_tap_03",
  "anim_memorymatch_pointbigleft_fast_01",
  "anim_dizzy_reaction_soft_02",
  "anim_speedtap_losegame_intensity03_01",
  "anim_memorymatch_successhand_player_03",
  "anim_energy_cubenotfound_01",
  "anim_sparking_earnsparks_01",
  "anim_pyramid_reacttocube_happy_high_01",
  "anim_dizzy_pickup_03",
  "anim_cozmosays_badword_01",
  "anim_energy_successgetout_01",
  "anim_energy_successgetout_02",
  "anim_rtpmemorymatch_yes_03",
  "anim_dizzy_pickup_01",
  "anim_cozmosays_getout_short_01_head_angle_40",
  "anim_cozmosays_getout_medium_01_head_angle_-20",
  "anim_cozmosays_app_getout_01",
  "anim_memorymatch_solo_reacttopattern_02",
  "anim_dizzy_reaction_medium_02",
  "anim_cozmosays_badword_01_head_angle_40",
  "anim_codelab_lightshow_idle",
  "anim_guarddog_fakeout_03",
  "anim_greeting_happy_01",
  "anim_cozmosays_getout_long_01_head_angle_20",
  "anim_rtpkeepaway_ideatoplay_02",
  "anim_memorymatch_failgame_player_03",
  "anim_dizzy_shake_stop_01",
  "anim_energy_cubedownlvl1_02",
  "anim_reacttoblock_triestoreach_01",
  "anim_energy_drivegeout_01",
  "anim_dizzy_reaction_medium_03",
  "anim_cozmosays_getout_medium_01_head_angle_40",
  "anim_cozmosings_80_song_01",
  "anim_cozmosings_120_song_01",
  "anim_peekaboo_failgetout_01",
  "anim_gamesetup_getout_01",
  "anim_guarddog_pulse_01",
  "anim_energy_cubedown_01",
  "anim_cozmosays_getin_long_01_head_angle_40",
  "anim_memorymatch_idle_01",
  "anim_cozmosays_getout_long_01_head_angle_-20",
  "anim_cozmosays_getout_short_01_head_angle_-20",
  "anim_cozmosays_getin_long_01_head_angle_20",
  "anim_memorymatch_pointbigright_fast_02",
  "anim_energy_eat_01",
  "anim_energy_cubeshake_02",
  "anim_energy_cubeshakelvl1_03",
  "anim_pyramid_lookinplaceforfaces_medium_head_angle_20",
  "anim_cozmosays_getin_long_01_head_angle_-20",
  "anim_rtpmemorymatch_yes_04",
  "anim_reacttoblock_admirecubetower_01",
  "anim_cozmosays_getin_short_01_head_angle_-20",
  "anim_energy_wakeup_01",
  "anim_cozmosays_getin_medium_01_head_angle_20",
  "anim_rtpkeepaway_playerno_01",
  "anim_bored_event_02",
  "anim_cozmosays_getout_medium_01_head_angle_20",
  "anim_sparking_idle_01",
  "anim_cozmosays_getout_medium_01",
  "anim_workout_mediumenergy_trans_01",
  "anim_memorymatch_successhand_player_01",
  "anim_energy_idlel2_06",
  "anim_speedtap_winround_intensity02_01",
  "anim_bouncer_intoscore_01",
  "anim_play_requestplay_level2_02",
  "anim_bouncer_intoscore_03",
  "anim_memorymatch_failhand_01",
  "anim_cozmosays_getin_long_01",
  "anim_cozmosays_badword_02",
  "anim_cozmosays_app_getout_02",
  "anim_cozmosays_badword_01_head_angle_-20",
  "anim_memorymatch_pointsmallright_fast_01",
  "anim_codelab_kitchen_eating_01",
  "anim_bouncer_timeout_01",
  "anim_petdetection_cat_01_head_angle_-20",
  "anim_memorymatch_failhand_02",
  "anim_memorymatch_reacttopattern_struggle_01",
  "anim_energy_fail_04",
  "anim_energy_idlel2_05",
  "anim_energy_cubedownlvl2_03",
  "anim_memorymatch_pointsmallleft_01",
  "anim_bouncer_intoscore_02",
  "anim_energy_cubeshake_lv2_03",
  "anim_gamesetup_idle_01",
  "anim_cozmosays_getin_medium_01_head_angle_-20",
  "anim_energy_getin_01",
  "anim_bouncer_rerequest_01",
  "anim_repair_fix_idle_fullyfull_02",
  "anim_energy_eat_04",
  "anim_lookatdevice_icon",
  "anim_energy_idlel2_03",
  "anim_memorymatch_pointsmallleft_fast_02",
  "anim_dizzy_reaction_hard_01",
  "anim_pounce_reacttoobj_01_shorter",
  "anim_peekaboo_successgetout_01",
  "anim_rtpmemorymatch_no_02",
  "anim_energy_eat_02",
  "anim_guarddog_settle_01",
  "anim_qa_intervals_30frame",
  "anim_vc_listen_loop_01",
  
  // todo: remove this if block once (VIC-4929) is resolved, or these animations no longer contain
  // procedural faces, whichever comes first
  "anim_lookatphone_getin_01",
  "anim_lookatphone_getout_01",
};

TEST(AnimationTest, AnimationLoading)
{
  const auto kPathToDevData = "assets/dev_animation_data";
  const auto fullPath = cozmoContext->GetDataPlatform()->GetResourcePath(kPathToDevData);
  const bool useFullPath = true;
  const bool shouldRecurse = false;

  Robot robot(0, cozmoContext);
  auto platform = cozmoContext->GetDataPlatform();
  auto spritePaths = robot.GetComponent<DataAccessorComponent>().GetSpritePaths();
  auto spriteSequenceContainer = robot.GetComponent<DataAccessorComponent>().GetSpriteSequenceContainer();
  std::atomic<float> loadingCompleteRatio(0);
  std::atomic<bool>  abortLoad(false);

  CannedAnimationContainer jsonAnimContainer;
  // Load JSON animations
  {
    const std::vector<const char*> jsonExt = {"json"};
    auto jsonFilePaths = Util::FileUtils::FilesInDirectory(fullPath, useFullPath, jsonExt, shouldRecurse);

    CannedAnimationLoader animLoader(platform,
                                     spritePaths, spriteSequenceContainer, 
                                     loadingCompleteRatio, abortLoad);
    for(const auto& fullPath : jsonFilePaths){
      animLoader.LoadAnimationIntoContainer(fullPath, &jsonAnimContainer);
    }
  }

  // Load Binary animations
  CannedAnimationContainer binaryAnimContainer;
  {
    const std::vector<const char*> binExt = {"bin"};
    auto binaryFilePaths = Util::FileUtils::FilesInDirectory(fullPath, useFullPath, binExt, shouldRecurse);

    CannedAnimationLoader animLoader(platform,
                                     spritePaths, spriteSequenceContainer, 
                                     loadingCompleteRatio, abortLoad);
    for(const auto& fullPath : binaryFilePaths){
      animLoader.LoadAnimationIntoContainer(fullPath, &binaryAnimContainer);
    }
  }

  // Compare the maps
  const auto jsonAnimNames = jsonAnimContainer.GetAnimationNames();
  const auto binaryAnimNames = binaryAnimContainer.GetAnimationNames();
  
  const bool sameSize = (jsonAnimNames.size() == binaryAnimNames.size());
  if(!sameSize){
    PRINT_NAMED_WARNING("AnimationTest.AnimationLoading.SizesDoNotMatch",
                        "There are %zu json animations and %zu binary animations - \
                        may be an issue copying assets for test",
                        jsonAnimNames.size(), binaryAnimNames.size());
    for(const auto& name: jsonAnimNames){
      const auto it = std::find(binaryAnimNames.begin(), binaryAnimNames.end(), name);
      if(it == binaryAnimNames.end()){
        PRINT_NAMED_ERROR("AnimationTest.AnimationLoading.MissingAnim", 
                          "Animation %s exists in JSON format, but not binary",
                          name.c_str());
      }
    }
    for(const auto& name: binaryAnimNames){
      const auto it = std::find(jsonAnimNames.begin(), jsonAnimNames.end(), name);
      if(it == jsonAnimNames.end()){
        PRINT_NAMED_ERROR("AnimationTest.AnimationLoading.MissingAnim", 
                          "Animation %s exists in binary format, but not JSON",
                          name.c_str());
      }
    }
  }

  // Make sure animation contents are the same - use binary as source of truth
  // above checks will fail if there are animations missing from binary
  for(const auto& name: binaryAnimNames){
    const bool animExists = jsonAnimContainer.HasAnimation(name);
    if(!animExists){
      PRINT_NAMED_WARNING("AnimationTest.AnimationLoading.MissingJSONAnim - may be an issue copying assets for test",
                          "JSON container does not have animation %s",
                          name.c_str());
      continue;
    }
    Animation* binaryAnim = binaryAnimContainer.GetAnimation(name);
    Animation* jsonAnim   = jsonAnimContainer.GetAnimation(name);
    ASSERT_TRUE(binaryAnim != nullptr);
    ASSERT_TRUE(jsonAnim != nullptr);
    
    // Check to see if this is a legacy animation that has known differences in data due to casting/rounding
    if(kLegacyAnimationsWithCastDifferences.find(name) != kLegacyAnimationsWithCastDifferences.end()){
      continue;
    }
    
    const bool areAnimationsEquivalent = (*binaryAnim == *jsonAnim);
    EXPECT_TRUE(areAnimationsEquivalent);
    if(!areAnimationsEquivalent){
      PRINT_NAMED_ERROR("AnimationTest.AnimationLoading.AnimationsDoNotMatch",
                        "Binary animation and JSON animation %s do not match",
                        name.c_str());
    }
  }

} // AnimationTest.AnimationLoading


// Ensure that copying animations/keyframes doesn't result in any lost information
TEST(AnimationTest, AnimationCopying)
{
  const auto kPathToDevData = "assets/animations";
  const auto fullPath = cozmoContext->GetDataPlatform()->GetResourcePath(kPathToDevData);
  const bool useFullPath = true;
  const bool shouldRecurse = false;
  
  Robot robot(0, cozmoContext);
  auto platform = cozmoContext->GetDataPlatform();
  auto spritePaths = robot.GetComponent<DataAccessorComponent>().GetSpritePaths();
  auto spriteSequenceContainer = robot.GetComponent<DataAccessorComponent>().GetSpriteSequenceContainer();
  std::atomic<float> loadingCompleteRatio(0);
  std::atomic<bool>  abortLoad(false);
  
  
  // Load Binary animations
  CannedAnimationContainer binaryAnimContainer;
  {
    const std::vector<const char*> binExt = {"bin"};
    auto binaryFilePaths = Util::FileUtils::FilesInDirectory(fullPath, useFullPath, binExt, shouldRecurse);
    
    CannedAnimationLoader animLoader(platform,
                                     spritePaths, spriteSequenceContainer,
                                     loadingCompleteRatio, abortLoad);
    for(const auto& fullPath : binaryFilePaths){
      animLoader.LoadAnimationIntoContainer(fullPath, &binaryAnimContainer);
    }
  }
  
  
  // Copy each animation and make sure that the contents stay the same
  const auto binaryAnimNames = binaryAnimContainer.GetAnimationNames();

  for(const auto& name: binaryAnimNames){
    Animation* binaryAnim = binaryAnimContainer.GetAnimation(name);
    ASSERT_TRUE(binaryAnim != nullptr);
    
    Animation copy = *binaryAnim;
    
    const bool areAnimationsEquivalent = (*binaryAnim == copy);
    EXPECT_TRUE(areAnimationsEquivalent);
    if(!areAnimationsEquivalent){
      PRINT_NAMED_ERROR("AnimationTest.AnimationCopying.AnimationsDoNotMatch",
                        "Animation %s has data which does not copy properly",
                        name.c_str());
    }
  }
  
} // AnimationTest.AnimationCopying

