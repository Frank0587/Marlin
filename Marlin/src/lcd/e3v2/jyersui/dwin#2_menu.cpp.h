
/* Menu Item Config */

void CrealityDWINClass::Menu_Item_Handler(uint8_t menu, uint8_t item, bool draw/*=true*/) {
  uint8_t row = item - scrollpos;
  #if HAS_LEVELING
    static bool level_state;
  #endif
  switch (menu) {
    case Prepare:

      #define PREPARE_BACK 0
      #define PREPARE_MOVE (PREPARE_BACK + 1)
      #define PREPARE_DISABLE (PREPARE_MOVE + 1)
      #define PREPARE_HOME (PREPARE_DISABLE + 1)
      #define PREPARE_MANUALLEVEL (PREPARE_HOME + 1)
      #define PREPARE_ZOFFSET (PREPARE_MANUALLEVEL + ENABLED(HAS_ZOFFSET_ITEM))
      #define PREPARE_PREHEAT (PREPARE_ZOFFSET + ENABLED(HAS_PREHEAT))
      #define PREPARE_COOLDOWN (PREPARE_PREHEAT + EITHER(HAS_HOTEND, HAS_HEATED_BED))
      #define PREPARE_CHANGEFIL (PREPARE_COOLDOWN + ENABLED(ADVANCED_PAUSE_FEATURE))
      #define PREPARE_ACTIONCOMMANDS (PREPARE_CHANGEFIL + 1)
      #define PREPARE_TOTAL PREPARE_ACTIONCOMMANDS

      switch (item) {
        case PREPARE_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Main_Menu(1);
          break;
        case PREPARE_MOVE:
          if (draw)
            Draw_Menu_Item(row, ICON_Axis, F("Move"), nullptr, true);
          else
            Draw_Menu(Move);
          break;
        case PREPARE_DISABLE:
          if (draw)
            Draw_Menu_Item(row, ICON_CloseMotor, F("Disable Stepper"));
          else
            queue.inject(F("M84"));
          break;
        case PREPARE_HOME:
          if (draw)
            Draw_Menu_Item(row, ICON_SetHome, F("Homing"), nullptr, true);
          else
            Draw_Menu(HomeMenu);
          break;
        case PREPARE_MANUALLEVEL:
          if (draw)
            Draw_Menu_Item(row, ICON_PrintSize, F("Manual Leveling"), nullptr, true);
          else {
            if (axes_should_home()) {
              Popup_Handler(Home);
              gcode.home_all_axes(true);
            }
            #if HAS_LEVELING
              level_state = planner.leveling_active;
              set_bed_leveling_enabled(false);
            #endif
            Draw_Menu(ManualLevel);
          }
          break;

        #if HAS_ZOFFSET_ITEM
          case PREPARE_ZOFFSET:
            if (draw)
              Draw_Menu_Item(row, ICON_Zoffset, F("Z-Offset"), nullptr, true);
            else {
              #if HAS_LEVELING
                level_state = planner.leveling_active;
                set_bed_leveling_enabled(false);
              #endif
              Draw_Menu(ZOffset);
            }
            break;
        #endif

        #if HAS_PREHEAT
          case PREPARE_PREHEAT:
            if (draw)
              Draw_Menu_Item(row, ICON_Temperature, F("Preheat"), nullptr, true);
            else
              Draw_Menu(Preheat);
            break;
        #endif

        #if HAS_HOTEND || HAS_HEATED_BED
          case PREPARE_COOLDOWN:
            if (draw)
              Draw_Menu_Item(row, ICON_Cool, F("Cooldown"));
            else
              thermalManager.cooldown();
            break;
        #endif

        #if ENABLED(HOST_ACTION_COMMANDS)
          case PREPARE_ACTIONCOMMANDS:
          if (draw) {
            Draw_Menu_Item(row, ICON_SetHome, F("Host Actions"), nullptr, true);
          }
          else {
            Draw_Menu(HostActions);
          }
          break;
        #endif

        #if ENABLED(ADVANCED_PAUSE_FEATURE)
          case PREPARE_CHANGEFIL:
            if (draw) {
              Draw_Menu_Item(row, ICON_ResumeEEPROM, F("Change Filament")
                #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
                  , nullptr, true
                #endif
              );
            }
            else {
              #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
                Draw_Menu(ChangeFilament);
              #else
                if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp)
                  Popup_Handler(ETemp);
                else {
                  if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                    Popup_Handler(Heating);
                    thermalManager.wait_for_hotend(0);
                  }
                  Popup_Handler(FilChange);
                  sprintf_P(cmd, PSTR("M600 B1 R%i"), thermalManager.temp_hotend[0].target);
                  gcode.process_subcommands_now(cmd);
                }
              #endif
            }
            break;
        #endif
      }
      break;

    case HomeMenu:

      #define HOME_BACK  0
      #define HOME_ALL   (HOME_BACK + 1)
      #define HOME_X     (HOME_ALL + 1)
      #define HOME_Y     (HOME_X + 1)
      #define HOME_Z     (HOME_Y + 1)
      #define HOME_SET   (HOME_Z + 1)
      #define HOME_TOTAL HOME_SET

      switch (item) {
        case HOME_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Prepare, PREPARE_HOME);
          break;
        case HOME_ALL:
          if (draw)
            Draw_Menu_Item(row, ICON_Homing, F("Home All"));
          else {
            Popup_Handler(Home);
            gcode.home_all_axes(true);
            Redraw_Menu();
          }
          break;
        case HOME_X:
          if (draw)
            Draw_Menu_Item(row, ICON_MoveX, F("Home X"));
          else {
            Popup_Handler(Home);
            gcode.process_subcommands_now(F("G28 X"));
            planner.synchronize();
            Redraw_Menu();
          }
          break;
        case HOME_Y:
          if (draw)
            Draw_Menu_Item(row, ICON_MoveY, F("Home Y"));
          else {
            Popup_Handler(Home);
            gcode.process_subcommands_now(F("G28 Y"));
            planner.synchronize();
            Redraw_Menu();
          }
          break;
        case HOME_Z:
          if (draw)
            Draw_Menu_Item(row, ICON_MoveZ, F("Home Z"));
          else {
            Popup_Handler(Home);
            gcode.process_subcommands_now(F("G28 Z"));
            planner.synchronize();
            Redraw_Menu();
          }
          break;
        case HOME_SET:
          if (draw)
            Draw_Menu_Item(row, ICON_SetHome, F("Set Home Position"));
          else {
            gcode.process_subcommands_now(F("G92 X0 Y0 Z0"));
            AudioFeedback();
          }
          break;
      }
      break;

    case Move:

      #define MOVE_BACK 0
      #define MOVE_X (MOVE_BACK + 1)
      #define MOVE_Y (MOVE_X + 1)
      #define MOVE_Z (MOVE_Y + 1)
      #define MOVE_E (MOVE_Z + ENABLED(HAS_HOTEND))
      #define MOVE_P (MOVE_E + ENABLED(HAS_BED_PROBE))
      #define MOVE_LIVE (MOVE_P + 1)
      #define MOVE_TOTAL MOVE_LIVE

      switch (item) {
        case MOVE_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else {
            #if HAS_BED_PROBE
              probe_deployed = false;
              probe.set_deployed(probe_deployed);
            #endif
            Draw_Menu(Prepare, PREPARE_MOVE);
          }
          break;
        case MOVE_X:
          if (draw) {
            Draw_Menu_Item(row, ICON_MoveX, F("Move X"));
            Draw_Float(current_position.x, row, false);
          }
          else
            Modify_Value(current_position.x, X_MIN_POS, X_MAX_POS, 10);
          break;
        case MOVE_Y:
          if (draw) {
            Draw_Menu_Item(row, ICON_MoveY, F("Move Y"));
            Draw_Float(current_position.y, row);
          }
          else
            Modify_Value(current_position.y, Y_MIN_POS, Y_MAX_POS, 10);
          break;
        case MOVE_Z:
          if (draw) {
            Draw_Menu_Item(row, ICON_MoveZ, F("Move Z"));
            Draw_Float(current_position.z, row);
          }
          else
            Modify_Value(current_position.z, Z_MIN_POS, Z_MAX_POS, 10);
          break;

        #if HAS_HOTEND
          case MOVE_E:
            if (draw) {
              Draw_Menu_Item(row, ICON_Extruder, F("Extruder"));
              current_position.e = 0;
              sync_plan_position();
              Draw_Float(current_position.e, row);
            }
            else {
              if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp) {
                Popup_Handler(ETemp);
              }
              else {
                if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                  Popup_Handler(Heating);
                  thermalManager.wait_for_hotend(0);
                  Redraw_Menu();
                }
                current_position.e = 0;
                sync_plan_position();
                Modify_Value(current_position.e, -500, 500, 10);
              }
            }
          break;
        #endif // HAS_HOTEND

        #if HAS_BED_PROBE
          case MOVE_P:
            if (draw) {
              Draw_Menu_Item(row, ICON_StockConfiguration, F("Probe"));
              Draw_Checkbox(row, probe_deployed);
            }
            else {
              probe_deployed = !probe_deployed;
              probe.set_deployed(probe_deployed);
              Draw_Checkbox(row, probe_deployed);
            }
            break;
        #endif

        case MOVE_LIVE:
          if (draw) {
            Draw_Menu_Item(row, ICON_Axis, F("Live Movement"));
            Draw_Checkbox(row, livemove);
          }
          else {
            livemove = !livemove;
            Draw_Checkbox(row, livemove);
          }
          break;
      }
      break;
    case ManualLevel:

      #define MLEVEL_BACK 0
      #define MLEVEL_PROBE (MLEVEL_BACK + ENABLED(HAS_BED_PROBE))
      #define MLEVEL_BL (MLEVEL_PROBE + 1)
      #define MLEVEL_TL (MLEVEL_BL + 1)
      #define MLEVEL_TR (MLEVEL_TL + 1)
      #define MLEVEL_BR (MLEVEL_TR + 1)
      #define MLEVEL_C (MLEVEL_BR + 1)
      #define MLEVEL_ZPOS (MLEVEL_C + 1)
      #define MLEVEL_TOTAL MLEVEL_ZPOS

      static float mlev_z_pos = 0;
      static bool use_probe = false;

      switch (item) {
        case MLEVEL_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else {
            TERN_(HAS_LEVELING, set_bed_leveling_enabled(level_state));
            Draw_Menu(Prepare, PREPARE_MANUALLEVEL);
          }
          break;
        #if HAS_BED_PROBE
          case MLEVEL_PROBE:
            if (draw) {
              Draw_Menu_Item(row, ICON_Zoffset, F("Use Probe"));
              Draw_Checkbox(row, use_probe);
            }
            else {
              use_probe = !use_probe;
              Draw_Checkbox(row, use_probe);
              if (use_probe) {
                Popup_Handler(Level);
                corner_avg = 0;
                #define PROBE_X_MIN _MAX(0 + corner_pos, X_MIN_POS + probe.offset.x, X_MIN_POS + PROBING_MARGIN) - probe.offset.x
                #define PROBE_X_MAX _MIN((X_BED_SIZE + X_MIN_POS) - corner_pos, X_MAX_POS + probe.offset.x, X_MAX_POS - PROBING_MARGIN) - probe.offset.x
                #define PROBE_Y_MIN _MAX(0 + corner_pos, Y_MIN_POS + probe.offset.y, Y_MIN_POS + PROBING_MARGIN) - probe.offset.y
                #define PROBE_Y_MAX _MIN((Y_BED_SIZE + Y_MIN_POS) - corner_pos, Y_MAX_POS + probe.offset.y, Y_MAX_POS - PROBING_MARGIN) - probe.offset.y
                corner_avg += probe.probe_at_point(PROBE_X_MIN, PROBE_Y_MIN, PROBE_PT_RAISE, 0, false);
                corner_avg += probe.probe_at_point(PROBE_X_MIN, PROBE_Y_MAX, PROBE_PT_RAISE, 0, false);
                corner_avg += probe.probe_at_point(PROBE_X_MAX, PROBE_Y_MAX, PROBE_PT_RAISE, 0, false);
                corner_avg += probe.probe_at_point(PROBE_X_MAX, PROBE_Y_MIN, PROBE_PT_STOW, 0, false);
                corner_avg /= 4;
                Redraw_Menu();
              }
            }
            break;
        #endif
        case MLEVEL_BL:
          if (draw)
            Draw_Menu_Item(row, ICON_AxisBL, F("Bottom Left"));
          else {
            Popup_Handler(MoveWait);
            if (use_probe) {
              #if HAS_BED_PROBE
                sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s"), dtostrf(PROBE_X_MIN, 1, 3, str_1), dtostrf(PROBE_Y_MIN, 1, 3, str_2));
                gcode.process_subcommands_now(cmd);
                planner.synchronize();
                Popup_Handler(ManualProbing);
              #endif
            }
            else {
              sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s\nG0 F300 Z%s"), dtostrf(corner_pos, 1, 3, str_1), dtostrf(corner_pos, 1, 3, str_2), dtostrf(mlev_z_pos, 1, 3, str_3));
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
          }
          break;
        case MLEVEL_TL:
          if (draw)
            Draw_Menu_Item(row, ICON_AxisTL, F("Top Left"));
          else {
            Popup_Handler(MoveWait);
            if (use_probe) {
              #if HAS_BED_PROBE
                sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s"), dtostrf(PROBE_X_MIN, 1, 3, str_1), dtostrf(PROBE_Y_MAX, 1, 3, str_2));
                gcode.process_subcommands_now(cmd);
                planner.synchronize();
                Popup_Handler(ManualProbing);
              #endif
            }
            else {
              sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s\nG0 F300 Z%s"), dtostrf(corner_pos, 1, 3, str_1), dtostrf((Y_BED_SIZE + Y_MIN_POS) - corner_pos, 1, 3, str_2), dtostrf(mlev_z_pos, 1, 3, str_3));
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
          }
          break;
        case MLEVEL_TR:
          if (draw)
            Draw_Menu_Item(row, ICON_AxisTR, F("Top Right"));
          else {
            Popup_Handler(MoveWait);
            if (use_probe) {
              #if HAS_BED_PROBE
                sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s"), dtostrf(PROBE_X_MAX, 1, 3, str_1), dtostrf(PROBE_Y_MAX, 1, 3, str_2));
                gcode.process_subcommands_now(cmd);
                planner.synchronize();
                Popup_Handler(ManualProbing);
              #endif
            }
            else {
              sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s\nG0 F300 Z%s"), dtostrf((X_BED_SIZE + X_MIN_POS) - corner_pos, 1, 3, str_1), dtostrf((Y_BED_SIZE + Y_MIN_POS) - corner_pos, 1, 3, str_2), dtostrf(mlev_z_pos, 1, 3, str_3));
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
          }
          break;
        case MLEVEL_BR:
          if (draw)
            Draw_Menu_Item(row, ICON_AxisBR, F("Bottom Right"));
          else {
            Popup_Handler(MoveWait);
            if (use_probe) {
              #if HAS_BED_PROBE
                sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s"), dtostrf(PROBE_X_MAX, 1, 3, str_1), dtostrf(PROBE_Y_MIN, 1, 3, str_2));
                gcode.process_subcommands_now(cmd);
                planner.synchronize();
                Popup_Handler(ManualProbing);
              #endif
            }
            else {
              sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s\nG0 F300 Z%s"), dtostrf((X_BED_SIZE + X_MIN_POS) - corner_pos, 1, 3, str_1), dtostrf(corner_pos, 1, 3, str_2), dtostrf(mlev_z_pos, 1, 3, str_3));
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
          }
          break;
        case MLEVEL_C:
          if (draw)
            Draw_Menu_Item(row, ICON_AxisC, F("Center"));
          else {
            Popup_Handler(MoveWait);
            if (use_probe) {
              #if HAS_BED_PROBE
                sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s"), dtostrf((X_BED_SIZE + X_MIN_POS) / 2.0f - probe.offset.x, 1, 3, str_1), dtostrf((Y_BED_SIZE + Y_MIN_POS) / 2.0f - probe.offset.y, 1, 3, str_2));
                gcode.process_subcommands_now(cmd);
                planner.synchronize();
                Popup_Handler(ManualProbing);
              #endif
            }
            else {
              sprintf_P(cmd, PSTR("G0 F4000\nG0 Z10\nG0 X%s Y%s\nG0 F300 Z%s"), dtostrf((X_BED_SIZE + X_MIN_POS) / 2.0f, 1, 3, str_1), dtostrf((Y_BED_SIZE + Y_MIN_POS) / 2.0f, 1, 3, str_2), dtostrf(mlev_z_pos, 1, 3, str_3));
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
          }
          break;
        case MLEVEL_ZPOS:
          if (draw) {
            Draw_Menu_Item(row, ICON_SetZOffset, F("Z Position"));
            Draw_Float(mlev_z_pos, row, false, 100);
          }
          else
            Modify_Value(mlev_z_pos, 0, MAX_Z_OFFSET, 100);
          break;
      }
      break;
    #if HAS_ZOFFSET_ITEM
      case ZOffset:

        #define ZOFFSET_BACK 0
        #define ZOFFSET_HOME (ZOFFSET_BACK + 1)
        #define ZOFFSET_MODE (ZOFFSET_HOME + 1)
        #define ZOFFSET_OFFSET (ZOFFSET_MODE + 1)
        #define ZOFFSET_UP (ZOFFSET_OFFSET + 1)
        #define ZOFFSET_DOWN (ZOFFSET_UP + 1)
        #define ZOFFSET_SAVE (ZOFFSET_DOWN + ENABLED(EEPROM_SETTINGS))
        #define ZOFFSET_TOTAL ZOFFSET_SAVE

        switch (item) {
          case ZOFFSET_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else {
              liveadjust = false;
              TERN_(HAS_LEVELING, set_bed_leveling_enabled(level_state));
              Draw_Menu(Prepare, PREPARE_ZOFFSET);
            }
            break;
          case ZOFFSET_HOME:
            if (draw)
              Draw_Menu_Item(row, ICON_Homing, F("Home Z Axis"));
            else {
              Popup_Handler(Home);
              gcode.process_subcommands_now(F("G28 Z"));
              Popup_Handler(MoveWait);
              #if ENABLED(Z_SAFE_HOMING)
                planner.synchronize();
                sprintf_P(cmd, PSTR("G0 F4000 X%s Y%s"), dtostrf(Z_SAFE_HOMING_X_POINT, 1, 3, str_1), dtostrf(Z_SAFE_HOMING_Y_POINT, 1, 3, str_2));
                gcode.process_subcommands_now(cmd);
              #else
                gcode.process_subcommands_now(F("G0 F4000 X117.5 Y117.5"));
              #endif
              gcode.process_subcommands_now(F("G0 F300 Z0"));
              planner.synchronize();
              Redraw_Menu();
            }
            break;
          case ZOFFSET_MODE:
            if (draw) {
              Draw_Menu_Item(row, ICON_Zoffset, F("Live Adjustment"));
              Draw_Checkbox(row, liveadjust);
            }
            else {
              if (!liveadjust) {
                if (axes_should_home()) {
                  Popup_Handler(Home);
                  gcode.home_all_axes(true);
                }
                Popup_Handler(MoveWait);
                #if ENABLED(Z_SAFE_HOMING)
                  planner.synchronize();
                  sprintf_P(cmd, PSTR("G0 F4000 X%s Y%s"), dtostrf(Z_SAFE_HOMING_X_POINT, 1, 3, str_1), dtostrf(Z_SAFE_HOMING_Y_POINT, 1, 3, str_2));
                  gcode.process_subcommands_now(cmd);
                #else
                  gcode.process_subcommands_now(F("G0 F4000 X117.5 Y117.5"));
                #endif
                gcode.process_subcommands_now(F("G0 F300 Z0"));
                planner.synchronize();
                Redraw_Menu();
              }
              liveadjust = !liveadjust;
              Draw_Checkbox(row, liveadjust);
            }
            break;
          case ZOFFSET_OFFSET:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetZOffset, F("Z Offset"));
              Draw_Float(zoffsetvalue, row, false, 100);
            }
            else
              Modify_Value(zoffsetvalue, MIN_Z_OFFSET, MAX_Z_OFFSET, 100);
            break;
          case ZOFFSET_UP:
            if (draw)
              Draw_Menu_Item(row, ICON_Axis, F("Microstep Up"));
            else {
              if (zoffsetvalue < MAX_Z_OFFSET) {
                if (liveadjust) {
                  gcode.process_subcommands_now(F("M290 Z0.01"));
                  planner.synchronize();
                }
                zoffsetvalue += 0.01;
                Draw_Float(zoffsetvalue, row - 1, false, 100);
              }
            }
            break;
          case ZOFFSET_DOWN:
            if (draw)
              Draw_Menu_Item(row, ICON_AxisD, F("Microstep Down"));
            else {
              if (zoffsetvalue > MIN_Z_OFFSET) {
                if (liveadjust) {
                  gcode.process_subcommands_now(F("M290 Z-0.01"));
                  planner.synchronize();
                }
                zoffsetvalue -= 0.01;
                Draw_Float(zoffsetvalue, row - 2, false, 100);
              }
            }
            break;
          #if ENABLED(EEPROM_SETTINGS)
            case ZOFFSET_SAVE:
              if (draw)
                Draw_Menu_Item(row, ICON_WriteEEPROM, F("Save"));
              else
                AudioFeedback(settings.save());
              break;
          #endif
        }
        break;
    #endif

    #if HAS_PREHEAT
      case Preheat: {
        #define PREHEAT_BACK 0
        #define PREHEAT_MODE (PREHEAT_BACK + 1)
        #define PREHEAT_1 (PREHEAT_MODE + 1)
        #define PREHEAT_2 (PREHEAT_1 + (PREHEAT_COUNT >= 2))
        #define PREHEAT_3 (PREHEAT_2 + (PREHEAT_COUNT >= 3))
        #define PREHEAT_4 (PREHEAT_3 + (PREHEAT_COUNT >= 4))
        #define PREHEAT_5 (PREHEAT_4 + (PREHEAT_COUNT >= 5))
        #define PREHEAT_TOTAL PREHEAT_5

        auto do_preheat = [](const uint8_t m) {
          thermalManager.cooldown();
          if (preheatmode == 0 || preheatmode == 1) { ui.preheat_hotend_and_fan(m); }
          if (preheatmode == 0 || preheatmode == 2) ui.preheat_bed(m);
        };

        switch (item) {
          case PREHEAT_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(Prepare, PREPARE_PREHEAT);
            break;
          case PREHEAT_MODE:
            if (draw) {
              Draw_Menu_Item(row, ICON_Homing, F("Preheat Mode"));
              Draw_Option(preheatmode, preheat_modes, row);
            }
            else
              Modify_Option(preheatmode, preheat_modes, 2);
            break;

          #if PREHEAT_COUNT >= 1
            case PREHEAT_1:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_1_LABEL));
              else
                do_preheat(0);
              break;
          #endif

          #if PREHEAT_COUNT >= 2
            case PREHEAT_2:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_2_LABEL));
              else
                do_preheat(1);
              break;
          #endif

          #if PREHEAT_COUNT >= 3
            case PREHEAT_3:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_3_LABEL));
              else
                do_preheat(2);
              break;
          #endif

          #if PREHEAT_COUNT >= 4
            case PREHEAT_4:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_4_LABEL));
              else
                do_preheat(3);
              break;
          #endif

          #if PREHEAT_COUNT >= 5
            case PREHEAT_5:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_5_LABEL));
              else
                do_preheat(4);
              break;
          #endif
        }
      } break;
    #endif // HAS_PREHEAT

    #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
      case ChangeFilament:

        #define CHANGEFIL_BACK 0
        #define CHANGEFIL_LOAD (CHANGEFIL_BACK + 1)
        #define CHANGEFIL_UNLOAD (CHANGEFIL_LOAD + 1)
        #define CHANGEFIL_CHANGE (CHANGEFIL_UNLOAD + 1)
        #define CHANGEFIL_TOTAL CHANGEFIL_CHANGE

        switch (item) {
          case CHANGEFIL_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(Prepare, PREPARE_CHANGEFIL);
            break;
          case CHANGEFIL_LOAD:
            if (draw)
              Draw_Menu_Item(row, ICON_WriteEEPROM, F("Load Filament"));
            else {
              if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp)
                Popup_Handler(ETemp);
              else {
                if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                  Popup_Handler(Heating);
                  thermalManager.wait_for_hotend(0);
                }
                Popup_Handler(FilLoad);
                gcode.process_subcommands_now(F("M701"));
                planner.synchronize();
                Redraw_Menu();
              }
            }
            break;
          case CHANGEFIL_UNLOAD:
            if (draw)
              Draw_Menu_Item(row, ICON_ReadEEPROM, F("Unload Filament"));
            else {
              if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp) {
                Popup_Handler(ETemp);
              }
              else {
                if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                  Popup_Handler(Heating);
                  thermalManager.wait_for_hotend(0);
                }
                Popup_Handler(FilLoad, true);
                gcode.process_subcommands_now(F("M702"));
                planner.synchronize();
                Redraw_Menu();
              }
            }
            break;
          case CHANGEFIL_CHANGE:
            if (draw)
              Draw_Menu_Item(row, ICON_ResumeEEPROM, F("Change Filament"));
            else {
              if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp)
                Popup_Handler(ETemp);
              else {
                if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                  Popup_Handler(Heating);
                  thermalManager.wait_for_hotend(0);
                }
                Popup_Handler(FilChange);
                sprintf_P(cmd, PSTR("M600 B1 R%i"), thermalManager.temp_hotend[0].target);
                gcode.process_subcommands_now(cmd);
              }
            }
            break;
        }
        break;
    #endif // FILAMENT_LOAD_UNLOAD_GCODES

    #if ENABLED(HOST_ACTION_COMMANDS)
      case HostActions:

        #define HOSTACTIONS_BACK 0
        #define HOSTACTIONS_1 (HOSTACTIONS_BACK + 1)
        #define HOSTACTIONS_2 (HOSTACTIONS_1 + 1)
        #define HOSTACTIONS_3 (HOSTACTIONS_2 + 1)
        #define HOSTACTIONS_TOTAL HOSTACTIONS_3

        switch(item) {
          case HOSTACTIONS_BACK:
            if (draw) {
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            }
            else {
              Draw_Menu(Prepare, PREPARE_ACTIONCOMMANDS);
            }
            break;
          case HOSTACTIONS_1:
            if (draw) {
              Draw_Menu_Item(row, ICON_File, action1);
            }
            else {
              if (!strcmp(action1, "-") == 0) hostui.action(F(action1));
            }
            break;
          case HOSTACTIONS_2:
            if (draw) {
              Draw_Menu_Item(row, ICON_File, action2);
            }
            else {
              if (!strcmp(action2, "-") == 0) hostui.action(F(action2));
            }
            break;
          case HOSTACTIONS_3:
            if (draw) {
              Draw_Menu_Item(row, ICON_File, action3);
            }
            else {
              if (!strcmp(action3, "-") == 0) hostui.action(F(action3));
            }
            break;
        }
        break;
    #endif

    case Control:

      #define CONTROL_BACK 0
      #define CONTROL_TEMP (CONTROL_BACK + 1)
      #define CONTROL_MOTION (CONTROL_TEMP + 1)
      #define CONTROL_VISUAL (CONTROL_MOTION + 1)
      #define CONTROL_HOSTSETTINGS (CONTROL_VISUAL + 1)
      #define CONTROL_ADVANCED (CONTROL_HOSTSETTINGS + 1)
      #define CONTROL_SAVE (CONTROL_ADVANCED + ENABLED(EEPROM_SETTINGS))
      #define CONTROL_RESTORE (CONTROL_SAVE + ENABLED(EEPROM_SETTINGS))
      #define CONTROL_RESET (CONTROL_RESTORE + ENABLED(EEPROM_SETTINGS))
      #define CONTROL_INFO (CONTROL_RESET + 1)
      #define CONTROL_TOTAL CONTROL_INFO

      switch (item) {
        case CONTROL_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Main_Menu(2);
          break;
        case CONTROL_TEMP:
          if (draw)
            Draw_Menu_Item(row, ICON_Temperature, F("Temperature"), nullptr, true);
          else
            Draw_Menu(TempMenu);
          break;
        case CONTROL_MOTION:
          if (draw)
            Draw_Menu_Item(row, ICON_Motion, F("Motion"), nullptr, true);
          else
            Draw_Menu(Motion);
          break;
        case CONTROL_VISUAL:
          if (draw)
            Draw_Menu_Item(row, ICON_PrintSize, F("Visual"), nullptr, true);
          else
            Draw_Menu(Visual);
          break;
        case CONTROL_HOSTSETTINGS:
          if (draw) {
            Draw_Menu_Item(row, ICON_Contact, F("Host Settings"), nullptr, true);
          }
          else {
            Draw_Menu(HostSettings);
          }
          break;
        case CONTROL_ADVANCED:
          if (draw)
            Draw_Menu_Item(row, ICON_Version, F("Advanced"), nullptr, true);
          else
            Draw_Menu(Advanced);
          break;
        #if ENABLED(EEPROM_SETTINGS)
          case CONTROL_SAVE:
            if (draw)
              Draw_Menu_Item(row, ICON_WriteEEPROM, F("Store Settings"));
            else
              AudioFeedback(settings.save());
            break;
          case CONTROL_RESTORE:
            if (draw)
              Draw_Menu_Item(row, ICON_ReadEEPROM, F("Restore Settings"));
            else
              AudioFeedback(settings.load());
            break;
          case CONTROL_RESET:
            if (draw)
              Draw_Menu_Item(row, ICON_Temperature, F("Reset to Defaults"));
            else {
              settings.reset();
              AudioFeedback();
            }
            break;
        #endif
        case CONTROL_INFO:
          if (draw)
            Draw_Menu_Item(row, ICON_Info, F("Info"));
          else
            Draw_Menu(Info);
          break;
      }
      break;

    case TempMenu:

      #define TEMP_BACK 0
      #define TEMP_HOTEND (TEMP_BACK + ENABLED(HAS_HOTEND))
      #define TEMP_BED (TEMP_HOTEND + ENABLED(HAS_HEATED_BED))
      #define TEMP_FAN (TEMP_BED + ENABLED(HAS_FAN))
      #define TEMP_PID (TEMP_FAN + ANY(HAS_HOTEND, HAS_HEATED_BED))
      #define TEMP_PREHEAT1 (TEMP_PID + (PREHEAT_COUNT >= 1))
      #define TEMP_PREHEAT2 (TEMP_PREHEAT1 + (PREHEAT_COUNT >= 2))
      #define TEMP_PREHEAT3 (TEMP_PREHEAT2 + (PREHEAT_COUNT >= 3))
      #define TEMP_PREHEAT4 (TEMP_PREHEAT3 + (PREHEAT_COUNT >= 4))
      #define TEMP_PREHEAT5 (TEMP_PREHEAT4 + (PREHEAT_COUNT >= 5))
      #define TEMP_TOTAL TEMP_PREHEAT5

      switch (item) {
        case TEMP_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Control, CONTROL_TEMP);
          break;
        #if HAS_HOTEND
          case TEMP_HOTEND:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
              Draw_Float(thermalManager.temp_hotend[0].target, row, false, 1);
            }
            else
              Modify_Value(thermalManager.temp_hotend[0].target, MIN_E_TEMP, MAX_E_TEMP, 1);
            break;
        #endif
        #if HAS_HEATED_BED
          case TEMP_BED:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
              Draw_Float(thermalManager.temp_bed.target, row, false, 1);
            }
            else
              Modify_Value(thermalManager.temp_bed.target, MIN_BED_TEMP, MAX_BED_TEMP, 1);
            break;
        #endif
        #if HAS_FAN
          case TEMP_FAN:
            if (draw) {
              Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
              Draw_Float(thermalManager.fan_speed[0], row, false, 1);
            }
            else
              Modify_Value(thermalManager.fan_speed[0], MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
            break;
        #endif
        #if HAS_HOTEND || HAS_HEATED_BED
          case TEMP_PID:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F("PID"), nullptr, true);
            else
              Draw_Menu(PID);
            break;
        #endif
        #if PREHEAT_COUNT >= 1
          case TEMP_PREHEAT1:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F(PREHEAT_1_LABEL), nullptr, true);
            else
              Draw_Menu(Preheat1);
            break;
        #endif
        #if PREHEAT_COUNT >= 2
          case TEMP_PREHEAT2:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F(PREHEAT_2_LABEL), nullptr, true);
            else
              Draw_Menu(Preheat2);
            break;
        #endif
        #if PREHEAT_COUNT >= 3
          case TEMP_PREHEAT3:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F(PREHEAT_3_LABEL), nullptr, true);
            else
              Draw_Menu(Preheat3);
            break;
        #endif
        #if PREHEAT_COUNT >= 4
          case TEMP_PREHEAT4:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F(PREHEAT_4_LABEL), nullptr, true);
            else
              Draw_Menu(Preheat4);
            break;
        #endif
        #if PREHEAT_COUNT >= 5
          case TEMP_PREHEAT5:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F(PREHEAT_5_LABEL), nullptr, true);
            else
              Draw_Menu(Preheat5);
            break;
        #endif
      }
      break;

    #if HAS_HOTEND || HAS_HEATED_BED
      case PID:

        #define PID_BACK 0
        #define PID_HOTEND (PID_BACK + ENABLED(HAS_HOTEND))
        #define PID_BED (PID_HOTEND + ENABLED(HAS_HEATED_BED))
        #define PID_CYCLES (PID_BED + 1)
        #define PID_TOTAL PID_CYCLES

        static uint8_t PID_cycles = 5;

        switch (item) {
          case PID_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(TempMenu, TEMP_PID);
            break;
          #if HAS_HOTEND
            case PID_HOTEND:
              if (draw)
                Draw_Menu_Item(row, ICON_HotendTemp, F("Hotend"), nullptr, true);
              else
                Draw_Menu(HotendPID);
              break;
          #endif
          #if HAS_HEATED_BED
            case PID_BED:
              if (draw)
                Draw_Menu_Item(row, ICON_BedTemp, F("Bed"), nullptr, true);
              else
                Draw_Menu(BedPID);
              break;
          #endif
          case PID_CYCLES:
            if (draw) {
              Draw_Menu_Item(row, ICON_FanSpeed, F("Cycles"));
              Draw_Float(PID_cycles, row, false, 1);
            }
            else
              Modify_Value(PID_cycles, 3, 50, 1);
            break;
        }
        break;
    #endif // HAS_HOTEND || HAS_HEATED_BED

    #if HAS_HOTEND
      case HotendPID:

        #define HOTENDPID_BACK 0
        #define HOTENDPID_TUNE (HOTENDPID_BACK + 1)
        #define HOTENDPID_TEMP (HOTENDPID_TUNE + 1)
        #define HOTENDPID_KP (HOTENDPID_TEMP + 1)
        #define HOTENDPID_KI (HOTENDPID_KP + 1)
        #define HOTENDPID_KD (HOTENDPID_KI + 1)
        #define HOTENDPID_TOTAL HOTENDPID_KD

        static uint16_t PID_e_temp = 180;

        switch (item) {
          case HOTENDPID_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(PID, PID_HOTEND);
            break;
          case HOTENDPID_TUNE:
            if (draw)
              Draw_Menu_Item(row, ICON_HotendTemp, F("Autotune"));
            else {
              Popup_Handler(PIDWait);
              sprintf_P(cmd, PSTR("M303 E0 C%i S%i U1"), PID_cycles, PID_e_temp);
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
            break;
          case HOTENDPID_TEMP:
            if (draw) {
              Draw_Menu_Item(row, ICON_Temperature, F("Temperature"));
              Draw_Float(PID_e_temp, row, false, 1);
            }
            else
              Modify_Value(PID_e_temp, MIN_E_TEMP, MAX_E_TEMP, 1);
            break;
          case HOTENDPID_KP:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("Kp Value"));
              Draw_Float(thermalManager.temp_hotend[0].pid.Kp, row, false, 100);
            }
            else
              Modify_Value(thermalManager.temp_hotend[0].pid.Kp, 0, 5000, 100, thermalManager.updatePID);
            break;
          case HOTENDPID_KI:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("Ki Value"));
              Draw_Float(unscalePID_i(thermalManager.temp_hotend[0].pid.Ki), row, false, 100);
            }
            else
              Modify_Value(thermalManager.temp_hotend[0].pid.Ki, 0, 5000, 100, thermalManager.updatePID);
            break;
          case HOTENDPID_KD:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("Kd Value"));
              Draw_Float(unscalePID_d(thermalManager.temp_hotend[0].pid.Kd), row, false, 100);
            }
            else
              Modify_Value(thermalManager.temp_hotend[0].pid.Kd, 0, 5000, 100, thermalManager.updatePID);
            break;
        }
        break;
    #endif // HAS_HOTEND

    #if HAS_HEATED_BED
      case BedPID:

        #define BEDPID_BACK 0
        #define BEDPID_TUNE (BEDPID_BACK + 1)
        #define BEDPID_TEMP (BEDPID_TUNE + 1)
        #define BEDPID_KP (BEDPID_TEMP + 1)
        #define BEDPID_KI (BEDPID_KP + 1)
        #define BEDPID_KD (BEDPID_KI + 1)
        #define BEDPID_TOTAL BEDPID_KD

        static uint16_t PID_bed_temp = 60;

        switch (item) {
          case BEDPID_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(PID, PID_BED);
            break;
          case BEDPID_TUNE:
            if (draw)
              Draw_Menu_Item(row, ICON_HotendTemp, F("Autotune"));
            else {
              Popup_Handler(PIDWait);
              sprintf_P(cmd, PSTR("M303 E-1 C%i S%i U1"), PID_cycles, PID_bed_temp);
              gcode.process_subcommands_now(cmd);
              planner.synchronize();
              Redraw_Menu();
            }
            break;
          case BEDPID_TEMP:
            if (draw) {
              Draw_Menu_Item(row, ICON_Temperature, F("Temperature"));
              Draw_Float(PID_bed_temp, row, false, 1);
            }
            else
              Modify_Value(PID_bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, 1);
            break;
          case BEDPID_KP:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("Kp Value"));
              Draw_Float(thermalManager.temp_bed.pid.Kp, row, false, 100);
            }
            else {
              Modify_Value(thermalManager.temp_bed.pid.Kp, 0, 5000, 100, thermalManager.updatePID);
            }
            break;
          case BEDPID_KI:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("Ki Value"));
              Draw_Float(unscalePID_i(thermalManager.temp_bed.pid.Ki), row, false, 100);
            }
            else
              Modify_Value(thermalManager.temp_bed.pid.Ki, 0, 5000, 100, thermalManager.updatePID);
            break;
          case BEDPID_KD:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("Kd Value"));
              Draw_Float(unscalePID_d(thermalManager.temp_bed.pid.Kd), row, false, 100);
            }
            else
              Modify_Value(thermalManager.temp_bed.pid.Kd, 0, 5000, 100, thermalManager.updatePID);
            break;
        }
        break;
    #endif // HAS_HEATED_BED

    #if PREHEAT_COUNT >= 1
      case Preheat1:

        #define PREHEAT1_BACK 0
        #define PREHEAT1_HOTEND (PREHEAT1_BACK + ENABLED(HAS_HOTEND))
        #define PREHEAT1_BED (PREHEAT1_HOTEND + ENABLED(HAS_HEATED_BED))
        #define PREHEAT1_FAN (PREHEAT1_BED + ENABLED(HAS_FAN))
        #define PREHEAT1_TOTAL PREHEAT1_FAN

        switch (item) {
          case PREHEAT1_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(TempMenu, TEMP_PREHEAT1);
            break;
          #if HAS_HOTEND
            case PREHEAT1_HOTEND:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
                Draw_Float(ui.material_preset[0].hotend_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[0].hotend_temp, MIN_E_TEMP, MAX_E_TEMP, 1);
              break;
          #endif
          #if HAS_HEATED_BED
            case PREHEAT1_BED:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
                Draw_Float(ui.material_preset[0].bed_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[0].bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, 1);
              break;
          #endif
          #if HAS_FAN
            case PREHEAT1_FAN:
              if (draw) {
                Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
                Draw_Float(ui.material_preset[0].fan_speed, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[0].fan_speed, MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
              break;
          #endif
        }
        break;
    #endif // PREHEAT_COUNT >= 1

    #if PREHEAT_COUNT >= 2
      case Preheat2:

        #define PREHEAT2_BACK 0
        #define PREHEAT2_HOTEND (PREHEAT2_BACK + ENABLED(HAS_HOTEND))
        #define PREHEAT2_BED (PREHEAT2_HOTEND + ENABLED(HAS_HEATED_BED))
        #define PREHEAT2_FAN (PREHEAT2_BED + ENABLED(HAS_FAN))
        #define PREHEAT2_TOTAL PREHEAT2_FAN

        switch (item) {
          case PREHEAT2_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(TempMenu, TEMP_PREHEAT2);
            break;
          #if HAS_HOTEND
            case PREHEAT2_HOTEND:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
                Draw_Float(ui.material_preset[1].hotend_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[1].hotend_temp, MIN_E_TEMP, MAX_E_TEMP, 1);
              break;
          #endif
          #if HAS_HEATED_BED
            case PREHEAT2_BED:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
                Draw_Float(ui.material_preset[1].bed_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[1].bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, 1);
              break;
          #endif
          #if HAS_FAN
            case PREHEAT2_FAN:
              if (draw) {
                Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
                Draw_Float(ui.material_preset[1].fan_speed, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[1].fan_speed, MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
              break;
          #endif
        }
        break;
    #endif // PREHEAT_COUNT >= 2

    #if PREHEAT_COUNT >= 3
      case Preheat3:

        #define PREHEAT3_BACK 0
        #define PREHEAT3_HOTEND (PREHEAT3_BACK + ENABLED(HAS_HOTEND))
        #define PREHEAT3_BED (PREHEAT3_HOTEND + ENABLED(HAS_HEATED_BED))
        #define PREHEAT3_FAN (PREHEAT3_BED + ENABLED(HAS_FAN))
        #define PREHEAT3_TOTAL PREHEAT3_FAN

        switch (item) {
          case PREHEAT3_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(TempMenu, TEMP_PREHEAT3);
            break;
          #if HAS_HOTEND
            case PREHEAT3_HOTEND:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
                Draw_Float(ui.material_preset[2].hotend_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[2].hotend_temp, MIN_E_TEMP, MAX_E_TEMP, 1);
              break;
          #endif
          #if HAS_HEATED_BED
            case PREHEAT3_BED:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
                Draw_Float(ui.material_preset[2].bed_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[2].bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, 1);
              break;
          #endif
          #if HAS_FAN
            case PREHEAT3_FAN:
              if (draw) {
                Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
                Draw_Float(ui.material_preset[2].fan_speed, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[2].fan_speed, MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
              break;
          #endif
        }
        break;
    #endif // PREHEAT_COUNT >= 3

    #if PREHEAT_COUNT >= 4
      case Preheat4:

        #define PREHEAT4_BACK 0
        #define PREHEAT4_HOTEND (PREHEAT4_BACK + ENABLED(HAS_HOTEND))
        #define PREHEAT4_BED (PREHEAT4_HOTEND + ENABLED(HAS_HEATED_BED))
        #define PREHEAT4_FAN (PREHEAT4_BED + ENABLED(HAS_FAN))
        #define PREHEAT4_TOTAL PREHEAT4_FAN

        switch (item) {
          case PREHEAT4_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(TempMenu, TEMP_PREHEAT4);
            break;
          #if HAS_HOTEND
            case PREHEAT4_HOTEND:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
                Draw_Float(ui.material_preset[3].hotend_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[3].hotend_temp, MIN_E_TEMP, MAX_E_TEMP, 1);
              break;
          #endif
          #if HAS_HEATED_BED
            case PREHEAT4_BED:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
                Draw_Float(ui.material_preset[3].bed_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[3].bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, 1);
              break;
          #endif
          #if HAS_FAN
            case PREHEAT4_FAN:
              if (draw) {
                Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
                Draw_Float(ui.material_preset[3].fan_speed, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[3].fan_speed, MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
              break;
          #endif
        }
        break;
    #endif // PREHEAT_COUNT >= 4

    #if PREHEAT_COUNT >= 5
      case Preheat5:

        #define PREHEAT5_BACK 0
        #define PREHEAT5_HOTEND (PREHEAT5_BACK + ENABLED(HAS_HOTEND))
        #define PREHEAT5_BED (PREHEAT5_HOTEND + ENABLED(HAS_HEATED_BED))
        #define PREHEAT5_FAN (PREHEAT5_BED + ENABLED(HAS_FAN))
        #define PREHEAT5_TOTAL PREHEAT5_FAN

        switch (item) {
          case PREHEAT5_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(TempMenu, TEMP_PREHEAT5);
            break;
          #if HAS_HOTEND
            case PREHEAT5_HOTEND:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
                Draw_Float(ui.material_preset[4].hotend_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[4].hotend_temp, MIN_E_TEMP, MAX_E_TEMP, 1);
              break;
          #endif
          #if HAS_HEATED_BED
            case PREHEAT5_BED:
              if (draw) {
                Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
                Draw_Float(ui.material_preset[4].bed_temp, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[4].bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, 1);
              break;
          #endif
          #if HAS_FAN
            case PREHEAT5_FAN:
              if (draw) {
                Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
                Draw_Float(ui.material_preset[4].fan_speed, row, false, 1);
              }
              else
                Modify_Value(ui.material_preset[4].fan_speed, MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
              break;
          #endif
        }
        break;
    #endif // PREHEAT_COUNT >= 5

    case Motion:

      #define MOTION_BACK 0
      #define MOTION_HOMEOFFSETS (MOTION_BACK + 1)
      #define MOTION_SPEED (MOTION_HOMEOFFSETS + 1)
      #define MOTION_ACCEL (MOTION_SPEED + 1)
      #define MOTION_JERK (MOTION_ACCEL + ENABLED(HAS_CLASSIC_JERK))
      #define MOTION_STEPS (MOTION_JERK + 1)
      #define MOTION_FLOW (MOTION_STEPS + ENABLED(HAS_HOTEND))
      #define MOTION_TOTAL MOTION_FLOW

      switch (item) {
        case MOTION_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Control, CONTROL_MOTION);
          break;
        case MOTION_HOMEOFFSETS:
          if (draw)
            Draw_Menu_Item(row, ICON_SetHome, F("Home Offsets"), nullptr, true);
          else
            Draw_Menu(HomeOffsets);
          break;
        case MOTION_SPEED:
          if (draw)
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Max Speed"), nullptr, true);
          else
            Draw_Menu(MaxSpeed);
          break;
        case MOTION_ACCEL:
          if (draw)
            Draw_Menu_Item(row, ICON_MaxAccelerated, F("Max Acceleration"), nullptr, true);
          else
            Draw_Menu(MaxAcceleration);
          break;
        #if HAS_CLASSIC_JERK
          case MOTION_JERK:
            if (draw)
              Draw_Menu_Item(row, ICON_MaxJerk, F("Max Jerk"), nullptr, true);
            else
              Draw_Menu(MaxJerk);
            break;
        #endif
        case MOTION_STEPS:
          if (draw)
            Draw_Menu_Item(row, ICON_Step, F("Steps/mm"), nullptr, true);
          else
            Draw_Menu(Steps);
          break;
        #if HAS_HOTEND
          case MOTION_FLOW:
            if (draw) {
              Draw_Menu_Item(row, ICON_Speed, F("Flow Rate"));
              Draw_Float(planner.flow_percentage[0], row, false, 1);
            }
            else
              Modify_Value(planner.flow_percentage[0], MIN_FLOW_RATE, MAX_FLOW_RATE, 1);
            break;
        #endif
      }
      break;

    case HomeOffsets:

      #define HOMEOFFSETS_BACK 0
      #define HOMEOFFSETS_XOFFSET (HOMEOFFSETS_BACK + 1)
      #define HOMEOFFSETS_YOFFSET (HOMEOFFSETS_XOFFSET + 1)
      #define HOMEOFFSETS_TOTAL HOMEOFFSETS_YOFFSET

      switch (item) {
        case HOMEOFFSETS_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Motion, MOTION_HOMEOFFSETS);
          break;
        case HOMEOFFSETS_XOFFSET:
          if (draw) {
            Draw_Menu_Item(row, ICON_StepX, F("X Offset"));
            Draw_Float(home_offset.x, row, false, 100);
          }
          else
            Modify_Value(home_offset.x, -MAX_XY_OFFSET, MAX_XY_OFFSET, 100);
          break;
        case HOMEOFFSETS_YOFFSET:
          if (draw) {
            Draw_Menu_Item(row, ICON_StepY, F("Y Offset"));
            Draw_Float(home_offset.y, row, false, 100);
          }
          else
            Modify_Value(home_offset.y, -MAX_XY_OFFSET, MAX_XY_OFFSET, 100);
          break;
      }
      break;
    case MaxSpeed:

      #define SPEED_BACK 0
      #define SPEED_X (SPEED_BACK + 1)
      #define SPEED_Y (SPEED_X + 1)
      #define SPEED_Z (SPEED_Y + 1)
      #define SPEED_E (SPEED_Z + ENABLED(HAS_HOTEND))
      #define SPEED_TOTAL SPEED_E

      switch (item) {
        case SPEED_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Motion, MOTION_SPEED);
          break;
        case SPEED_X:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeedX, F("X Axis"));
            Draw_Float(planner.settings.max_feedrate_mm_s[X_AXIS], row, false, 1);
          }
          else
            Modify_Value(planner.settings.max_feedrate_mm_s[X_AXIS], 0, default_max_feedrate[X_AXIS] * 2, 1);
          break;

        #if HAS_Y_AXIS
          case SPEED_Y:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxSpeedY, F("Y Axis"));
              Draw_Float(planner.settings.max_feedrate_mm_s[Y_AXIS], row, false, 1);
            }
            else
              Modify_Value(planner.settings.max_feedrate_mm_s[Y_AXIS], 0, default_max_feedrate[Y_AXIS] * 2, 1);
            break;
        #endif

        #if HAS_Z_AXIS
          case SPEED_Z:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxSpeedZ, F("Z Axis"));
              Draw_Float(planner.settings.max_feedrate_mm_s[Z_AXIS], row, false, 1);
            }
            else
              Modify_Value(planner.settings.max_feedrate_mm_s[Z_AXIS], 0, default_max_feedrate[Z_AXIS] * 2, 1);
            break;
        #endif

        #if HAS_HOTEND
          case SPEED_E:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxSpeedE, F("Extruder"));
              Draw_Float(planner.settings.max_feedrate_mm_s[E_AXIS], row, false, 1);
            }
            else
              Modify_Value(planner.settings.max_feedrate_mm_s[E_AXIS], 0, default_max_feedrate[E_AXIS] * 2, 1);
            break;
        #endif
      }
      break;

    case MaxAcceleration:

      #define ACCEL_BACK 0
      #define ACCEL_X (ACCEL_BACK + 1)
      #define ACCEL_Y (ACCEL_X + 1)
      #define ACCEL_Z (ACCEL_Y + 1)
      #define ACCEL_E (ACCEL_Z + ENABLED(HAS_HOTEND))
      #define ACCEL_TOTAL ACCEL_E

      switch (item) {
        case ACCEL_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Motion, MOTION_ACCEL);
          break;
        case ACCEL_X:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxAccX, F("X Axis"));
            Draw_Float(planner.settings.max_acceleration_mm_per_s2[X_AXIS], row, false, 1);
          }
          else
            Modify_Value(planner.settings.max_acceleration_mm_per_s2[X_AXIS], 0, default_max_acceleration[X_AXIS] * 2, 1);
          break;
        case ACCEL_Y:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxAccY, F("Y Axis"));
            Draw_Float(planner.settings.max_acceleration_mm_per_s2[Y_AXIS], row, false, 1);
          }
          else
            Modify_Value(planner.settings.max_acceleration_mm_per_s2[Y_AXIS], 0, default_max_acceleration[Y_AXIS] * 2, 1);
          break;
        case ACCEL_Z:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxAccZ, F("Z Axis"));
            Draw_Float(planner.settings.max_acceleration_mm_per_s2[Z_AXIS], row, false, 1);
          }
          else
            Modify_Value(planner.settings.max_acceleration_mm_per_s2[Z_AXIS], 0, default_max_acceleration[Z_AXIS] * 2, 1);
          break;
        #if HAS_HOTEND
          case ACCEL_E:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxAccE, F("Extruder"));
              Draw_Float(planner.settings.max_acceleration_mm_per_s2[E_AXIS], row, false, 1);
            }
            else
              Modify_Value(planner.settings.max_acceleration_mm_per_s2[E_AXIS], 0, default_max_acceleration[E_AXIS] * 2, 1);
            break;
        #endif
      }
      break;
    #if HAS_CLASSIC_JERK
      case MaxJerk:

        #define JERK_BACK 0
        #define JERK_X (JERK_BACK + 1)
        #define JERK_Y (JERK_X + 1)
        #define JERK_Z (JERK_Y + 1)
        #define JERK_E (JERK_Z + ENABLED(HAS_HOTEND))
        #define JERK_TOTAL JERK_E

        switch (item) {
          case JERK_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(Motion, MOTION_JERK);
            break;
          case JERK_X:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxSpeedJerkX, F("X Axis"));
              Draw_Float(planner.max_jerk[X_AXIS], row, false, 10);
            }
            else
              Modify_Value(planner.max_jerk[X_AXIS], 0, default_max_jerk[X_AXIS] * 2, 10);
            break;
          case JERK_Y:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxSpeedJerkY, F("Y Axis"));
              Draw_Float(planner.max_jerk[Y_AXIS], row, false, 10);
            }
            else
              Modify_Value(planner.max_jerk[Y_AXIS], 0, default_max_jerk[Y_AXIS] * 2, 10);
            break;
          case JERK_Z:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxSpeedJerkZ, F("Z Axis"));
              Draw_Float(planner.max_jerk[Z_AXIS], row, false, 10);
            }
            else
              Modify_Value(planner.max_jerk[Z_AXIS], 0, default_max_jerk[Z_AXIS] * 2, 10);
            break;
          #if HAS_HOTEND
            case JERK_E:
              if (draw) {
                Draw_Menu_Item(row, ICON_MaxSpeedJerkE, F("Extruder"));
                Draw_Float(planner.max_jerk[E_AXIS], row, false, 10);
              }
              else
                Modify_Value(planner.max_jerk[E_AXIS], 0, default_max_jerk[E_AXIS] * 2, 10);
              break;
          #endif
        }
        break;
    #endif
    case Steps:

      #define STEPS_BACK 0
      #define STEPS_X (STEPS_BACK + 1)
      #define STEPS_Y (STEPS_X + 1)
      #define STEPS_Z (STEPS_Y + 1)
      #define STEPS_E (STEPS_Z + ENABLED(HAS_HOTEND))
      #define STEPS_TOTAL STEPS_E

      switch (item) {
        case STEPS_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Motion, MOTION_STEPS);
          break;
        case STEPS_X:
          if (draw) {
            Draw_Menu_Item(row, ICON_StepX, F("X Axis"));
            Draw_Float(planner.settings.axis_steps_per_mm[X_AXIS], row, false, 10);
          }
          else
            Modify_Value(planner.settings.axis_steps_per_mm[X_AXIS], 0, default_steps[X_AXIS] * 2, 10);
          break;
        case STEPS_Y:
          if (draw) {
            Draw_Menu_Item(row, ICON_StepY, F("Y Axis"));
            Draw_Float(planner.settings.axis_steps_per_mm[Y_AXIS], row, false, 10);
          }
          else
            Modify_Value(planner.settings.axis_steps_per_mm[Y_AXIS], 0, default_steps[Y_AXIS] * 2, 10);
          break;
        case STEPS_Z:
          if (draw) {
            Draw_Menu_Item(row, ICON_StepZ, F("Z Axis"));
            Draw_Float(planner.settings.axis_steps_per_mm[Z_AXIS], row, false, 10);
          }
          else
            Modify_Value(planner.settings.axis_steps_per_mm[Z_AXIS], 0, default_steps[Z_AXIS] * 2, 10);
          break;
        #if HAS_HOTEND
          case STEPS_E:
            if (draw) {
              Draw_Menu_Item(row, ICON_StepE, F("Extruder"));
              Draw_Float(planner.settings.axis_steps_per_mm[E_AXIS], row, false, 10);
            }
            else
              Modify_Value(planner.settings.axis_steps_per_mm[E_AXIS], 0, 1000, 10);
            break;
        #endif
      }
      break;

    case Visual:

      #define VISUAL_BACK 0
      #define VISUAL_BACKLIGHT (VISUAL_BACK + 1)
      #define VISUAL_BRIGHTNESS (VISUAL_BACKLIGHT + 1)
      #define VISUAL_TIME_FORMAT (VISUAL_BRIGHTNESS + 1)
      #define VISUAL_COLOR_THEMES (VISUAL_TIME_FORMAT + 1)
      #define VISUAL_TOTAL VISUAL_COLOR_THEMES

      switch (item) {
        case VISUAL_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Control, CONTROL_VISUAL);
          break;
        case VISUAL_BACKLIGHT:
          if (draw)
            Draw_Menu_Item(row, ICON_Brightness, F("Display Off"));
          else
            ui.set_brightness(0);
          break;
        case VISUAL_BRIGHTNESS:
          if (draw) {
            Draw_Menu_Item(row, ICON_Brightness, F("LCD Brightness"));
            Draw_Float(ui.brightness, row, false, 1);
          }
          else
            Modify_Value(ui.brightness, LCD_BRIGHTNESS_MIN, LCD_BRIGHTNESS_MAX, 1, ui.refresh_brightness);
          break;
        case VISUAL_TIME_FORMAT:
          if (draw) {
            Draw_Menu_Item(row, ICON_PrintTime, F("Progress as __h__m"));
            Draw_Checkbox(row, eeprom_settings.time_format_textual);
          }
          else {
            eeprom_settings.time_format_textual = !eeprom_settings.time_format_textual;
            Draw_Checkbox(row, eeprom_settings.time_format_textual);
          }
          break;
        case VISUAL_COLOR_THEMES:
          if (draw)
            Draw_Menu_Item(row, ICON_MaxSpeed, F("UI Color Settings"), nullptr, true);
          else
            Draw_Menu(ColorSettings);
        break;
      }
      break;

    case ColorSettings:

      #define COLORSETTINGS_BACK 0
      #define COLORSETTINGS_CURSOR (COLORSETTINGS_BACK + 1)
      #define COLORSETTINGS_SPLIT_LINE (COLORSETTINGS_CURSOR + 1)
      #define COLORSETTINGS_MENU_TOP_TXT (COLORSETTINGS_SPLIT_LINE + 1)
      #define COLORSETTINGS_MENU_TOP_BG (COLORSETTINGS_MENU_TOP_TXT + 1)
      #define COLORSETTINGS_HIGHLIGHT_BORDER (COLORSETTINGS_MENU_TOP_BG + 1)
      #define COLORSETTINGS_PROGRESS_PERCENT (COLORSETTINGS_HIGHLIGHT_BORDER + 1)
      #define COLORSETTINGS_PROGRESS_TIME (COLORSETTINGS_PROGRESS_PERCENT + 1)
      #define COLORSETTINGS_PROGRESS_STATUS_BAR (COLORSETTINGS_PROGRESS_TIME + 1)
      #define COLORSETTINGS_PROGRESS_STATUS_AREA (COLORSETTINGS_PROGRESS_STATUS_BAR + 1)
      #define COLORSETTINGS_PROGRESS_COORDINATES (COLORSETTINGS_PROGRESS_STATUS_AREA + 1)
      #define COLORSETTINGS_PROGRESS_COORDINATES_LINE (COLORSETTINGS_PROGRESS_COORDINATES + 1)
      #define COLORSETTINGS_TOTAL COLORSETTINGS_PROGRESS_COORDINATES_LINE

      switch (item) {
        case COLORSETTINGS_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Visual, VISUAL_COLOR_THEMES);
          break;
        case COLORSETTINGS_CURSOR:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Cursor"));
            Draw_Option(eeprom_settings.cursor_color, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.cursor_color, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_SPLIT_LINE:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Menu Split Line"));
            Draw_Option(eeprom_settings.menu_split_line, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.menu_split_line, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_MENU_TOP_TXT:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Menu Header Text"));
            Draw_Option(eeprom_settings.menu_top_txt, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.menu_top_txt, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_MENU_TOP_BG:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Menu Header Bg"));
            Draw_Option(eeprom_settings.menu_top_bg, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.menu_top_bg, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_HIGHLIGHT_BORDER:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Highlight Box"));
            Draw_Option(eeprom_settings.highlight_box, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.highlight_box, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_PROGRESS_PERCENT:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Progress Percent"));
            Draw_Option(eeprom_settings.progress_percent, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.progress_percent, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_PROGRESS_TIME:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Progress Time"));
            Draw_Option(eeprom_settings.progress_time, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.progress_time, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_PROGRESS_STATUS_BAR:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Status Bar Text"));
            Draw_Option(eeprom_settings.status_bar_text, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.status_bar_text, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_PROGRESS_STATUS_AREA:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Status Area Text"));
            Draw_Option(eeprom_settings.status_area_text, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.status_area_text, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_PROGRESS_COORDINATES:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Coordinates Text"));
            Draw_Option(eeprom_settings.coordinates_text, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.coordinates_text, color_names, Custom_Colors);
          break;
        case COLORSETTINGS_PROGRESS_COORDINATES_LINE:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxSpeed, F("Coordinates Line"));
            Draw_Option(eeprom_settings.coordinates_split_line, color_names, row, false, true);
          }
          else
            Modify_Option(eeprom_settings.coordinates_split_line, color_names, Custom_Colors);
          break;
      } // switch (item)
      break;

    case HostSettings:

      #define HOSTSETTINGS_BACK 0
      #define HOSTSETTINGS_ACTIONCOMMANDS (HOSTSETTINGS_BACK + ENABLED(HOST_ACTION_COMMANDS))
      #define HOSTSETTINGS_TOTAL HOSTSETTINGS_ACTIONCOMMANDS

      switch (item) {
        case HOSTSETTINGS_BACK:
          if (draw) {
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          }
          else {
            Draw_Menu(Control, CONTROL_HOSTSETTINGS);
          }
          break;
        #if ENABLED(HOST_ACTION_COMMANDS)
          case HOSTSETTINGS_ACTIONCOMMANDS:
            if (draw) {
              Draw_Menu_Item(row, ICON_File, F("Action Commands"));
            }
            else {
              Draw_Menu(ActionCommands);
            }
            break;
        #endif
      }
      break;
    #if ENABLED(HOST_ACTION_COMMANDS)
    case ActionCommands:

      #define ACTIONCOMMANDS_BACK 0
      #define ACTIONCOMMANDS_1 (ACTIONCOMMANDS_BACK + 1)
      #define ACTIONCOMMANDS_2 (ACTIONCOMMANDS_1 + 1)
      #define ACTIONCOMMANDS_3 (ACTIONCOMMANDS_2 + 1)
      #define ACTIONCOMMANDS_TOTAL ACTIONCOMMANDS_3

      switch (item) {
        case ACTIONCOMMANDS_BACK:
          if (draw) {
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          }
          else {
            Draw_Menu(HostSettings, HOSTSETTINGS_ACTIONCOMMANDS);
          }
          break;
        case ACTIONCOMMANDS_1:
          if (draw) {
            Draw_Menu_Item(row, ICON_File, F("Action #1"));
            Draw_String(action1, row);
          }
          else {
            Modify_String(action1, 8, true);
          }
          break;
        case ACTIONCOMMANDS_2:
          if (draw) {
            Draw_Menu_Item(row, ICON_File, F("Action #2"));
            Draw_String(action2, row);
          }
          else {
            Modify_String(action2, 8, true);
          }
          break;
        case ACTIONCOMMANDS_3:
          if (draw) {
            Draw_Menu_Item(row, ICON_File, F("Action #3"));
            Draw_String(action3, row);
          }
          else {
            Modify_String(action3, 8, true);
          }
          break;
      }
      break;
    #endif

    case Advanced:

      #define ADVANCED_BACK 0
      #define ADVANCED_BEEPER (ADVANCED_BACK + ENABLED(SOUND_MENU_ITEM))
      #define ADVANCED_PROBE (ADVANCED_BEEPER + ENABLED(HAS_BED_PROBE))
      #define ADVANCED_CORNER (ADVANCED_PROBE + 1)
      #define ADVANCED_LA (ADVANCED_CORNER + ENABLED(LIN_ADVANCE))
      #define ADVANCED_LOAD (ADVANCED_LA + ENABLED(ADVANCED_PAUSE_FEATURE))
      #define ADVANCED_UNLOAD (ADVANCED_LOAD + ENABLED(ADVANCED_PAUSE_FEATURE))
      #define ADVANCED_COLD_EXTRUDE  (ADVANCED_UNLOAD + ENABLED(PREVENT_COLD_EXTRUSION))
      #define ADVANCED_FILSENSORENABLED (ADVANCED_COLD_EXTRUDE + ENABLED(FILAMENT_RUNOUT_SENSOR))
      #define ADVANCED_FILSENSORDISTANCE (ADVANCED_FILSENSORENABLED + ENABLED(HAS_FILAMENT_RUNOUT_DISTANCE))
      #define ADVANCED_POWER_LOSS (ADVANCED_FILSENSORDISTANCE + ENABLED(POWER_LOSS_RECOVERY))
      #define ADVANCED_TOTAL ADVANCED_POWER_LOSS

      switch (item) {
        case ADVANCED_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Menu(Control, CONTROL_ADVANCED);
          break;

        #if ENABLED(SOUND_MENU_ITEM)
          case ADVANCED_BEEPER:
            if (draw) {
              Draw_Menu_Item(row, ICON_Version, F("LCD Beeper"));
              Draw_Checkbox(row, ui.buzzer_enabled);
            }
            else {
              ui.buzzer_enabled = !ui.buzzer_enabled;
              Draw_Checkbox(row, ui.buzzer_enabled);
            }
            break;
        #endif

        #if HAS_BED_PROBE
          case ADVANCED_PROBE:
            if (draw)
              Draw_Menu_Item(row, ICON_StepX, F("Probe"), nullptr, true);
            else
              Draw_Menu(ProbeMenu);
            break;
        #endif

        case ADVANCED_CORNER:
          if (draw) {
            Draw_Menu_Item(row, ICON_MaxAccelerated, F("Bed Screw Inset"));
            Draw_Float(corner_pos, row, false, 10);
          }
          else
            Modify_Value(corner_pos, 1, 100, 10);
          break;

        #if ENABLED(LIN_ADVANCE)
          case ADVANCED_LA:
            if (draw) {
              Draw_Menu_Item(row, ICON_MaxAccelerated, F("Lin Advance Kp"));
              Draw_Float(planner.extruder_advance_K[0], row, false, 100);
            }
            else
              Modify_Value(planner.extruder_advance_K[0], 0, 10, 100);
            break;
        #endif

        #if ENABLED(ADVANCED_PAUSE_FEATURE)
          case ADVANCED_LOAD:
            if (draw) {
              Draw_Menu_Item(row, ICON_WriteEEPROM, F("Load Length"));
              Draw_Float(fc_settings[0].load_length, row, false, 1);
            }
            else
              Modify_Value(fc_settings[0].load_length, 0, EXTRUDE_MAXLENGTH, 1);
            break;
          case ADVANCED_UNLOAD:
            if (draw) {
              Draw_Menu_Item(row, ICON_ReadEEPROM, F("Unload Length"));
              Draw_Float(fc_settings[0].unload_length, row, false, 1);
            }
            else
              Modify_Value(fc_settings[0].unload_length, 0, EXTRUDE_MAXLENGTH, 1);
            break;
        #endif // ADVANCED_PAUSE_FEATURE

        #if ENABLED(PREVENT_COLD_EXTRUSION)
          case ADVANCED_COLD_EXTRUDE:
            if (draw) {
              Draw_Menu_Item(row, ICON_Cool, F("Min Extrusion T"));
              Draw_Float(thermalManager.extrude_min_temp, row, false, 1);
            }
            else {
              Modify_Value(thermalManager.extrude_min_temp, 0, MAX_E_TEMP, 1);
              thermalManager.allow_cold_extrude = (thermalManager.extrude_min_temp == 0);
            }
            break;
        #endif

        #if ENABLED(FILAMENT_RUNOUT_SENSOR)
          case ADVANCED_FILSENSORENABLED:
            if (draw) {
              Draw_Menu_Item(row, ICON_Extruder, F("Filament Sensor"));
              Draw_Checkbox(row, runout.enabled);
            }
            else {
              runout.enabled = !runout.enabled;
              Draw_Checkbox(row, runout.enabled);
            }
            break;

          #if ENABLED(HAS_FILAMENT_RUNOUT_DISTANCE)
            case ADVANCED_FILSENSORDISTANCE:
              if (draw) {
                Draw_Menu_Item(row, ICON_MaxAccE, F("Runout Distance"));
                Draw_Float(runout.runout_distance(), row, false, 10);
              }
              else
                Modify_Value(runout.runout_distance(), 0, 999, 10);
              break;
          #endif
        #endif // FILAMENT_RUNOUT_SENSOR

        #if ENABLED(POWER_LOSS_RECOVERY)
          case ADVANCED_POWER_LOSS:
            if (draw) {
              Draw_Menu_Item(row, ICON_Motion, F("Power-loss recovery"));
              Draw_Checkbox(row, recovery.enabled);
            }
            else {
              recovery.enable(!recovery.enabled);
              Draw_Checkbox(row, recovery.enabled);
            }
            break;
        #endif
      }
      break;

    #if HAS_BED_PROBE
      case ProbeMenu:

        #define PROBE_BACK 0
        #define PROBE_XOFFSET (PROBE_BACK + 1)
        #define PROBE_YOFFSET (PROBE_XOFFSET + 1)
        #define PROBE_TEST (PROBE_YOFFSET + 1)
        #define PROBE_TEST_COUNT (PROBE_TEST + 1)
        #define PROBE_TOTAL PROBE_TEST_COUNT

        static uint8_t testcount = 4;

        switch (item) {
          case PROBE_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(Advanced, ADVANCED_PROBE);
            break;

            case PROBE_XOFFSET:
              if (draw) {
                Draw_Menu_Item(row, ICON_StepX, F("Probe X Offset"));
                Draw_Float(probe.offset.x, row, false, 10);
              }
              else
                Modify_Value(probe.offset.x, -MAX_XY_OFFSET, MAX_XY_OFFSET, 10);
              break;
            case PROBE_YOFFSET:
              if (draw) {
                Draw_Menu_Item(row, ICON_StepY, F("Probe Y Offset"));
                Draw_Float(probe.offset.y, row, false, 10);
              }
              else
                Modify_Value(probe.offset.y, -MAX_XY_OFFSET, MAX_XY_OFFSET, 10);
              break;
            case PROBE_TEST:
              if (draw)
                Draw_Menu_Item(row, ICON_StepY, F("M48 Probe Test"));
              else {
                sprintf_P(cmd, PSTR("G28O\nM48 X%s Y%s P%i"), dtostrf((X_BED_SIZE + X_MIN_POS) / 2.0f, 1, 3, str_1), dtostrf((Y_BED_SIZE + Y_MIN_POS) / 2.0f, 1, 3, str_2), testcount);
                gcode.process_subcommands_now(cmd);
              }
              break;
            case PROBE_TEST_COUNT:
              if (draw) {
                Draw_Menu_Item(row, ICON_StepY, F("Probe Test Count"));
                Draw_Float(testcount, row, false, 1);
              }
              else
                Modify_Value(testcount, 4, 50, 1);
              break;
        }
        break;
    #endif

    case InfoMain:
    case Info:

      #define INFO_BACK 0
      #define INFO_PRINTCOUNT (INFO_BACK + ENABLED(PRINTCOUNTER))
      #define INFO_PRINTTIME (INFO_PRINTCOUNT + ENABLED(PRINTCOUNTER))
      #define INFO_SIZE (INFO_PRINTTIME + 1)
      #define INFO_VERSION (INFO_SIZE + 1)
      #define INFO_CONTACT (INFO_VERSION + 1)
      #define INFO_TOTAL INFO_BACK

      switch (item) {
        case INFO_BACK:
          if (draw) {
            Draw_Menu_Item(row, ICON_Back, F("Back"));

            #if ENABLED(PRINTCOUNTER)
              char row1[50], row2[50], buf[32];
              printStatistics ps = print_job_timer.getStats();

              sprintf_P(row1, PSTR("%i prints, %i finished"), ps.totalPrints, ps.finishedPrints);
              sprintf_P(row2, PSTR("%s m filament used"), dtostrf(ps.filamentUsed / 1000, 1, 2, str_1));
              Draw_Menu_Item(INFO_PRINTCOUNT, ICON_HotendTemp, row1, row2, false, true);

              duration_t(print_job_timer.getStats().printTime).toString(buf);
              sprintf_P(row1, PSTR("Printed: %s"), buf);
              duration_t(print_job_timer.getStats().longestPrint).toString(buf);
              sprintf_P(row2, PSTR("Longest: %s"), buf);
              Draw_Menu_Item(INFO_PRINTTIME, ICON_PrintTime, row1, row2, false, true);
            #endif

            Draw_Menu_Item(INFO_SIZE, ICON_PrintSize, F(MACHINE_SIZE), nullptr, false, true);
            Draw_Menu_Item(INFO_VERSION, ICON_Version, F(SHORT_BUILD_VERSION), nullptr, false, true);
            Draw_Menu_Item(INFO_CONTACT, ICON_Contact, F(CORP_WEBSITE), nullptr, false, true);
          }
          else {
            if (menu == Info)
              Draw_Menu(Control, CONTROL_INFO);
            else
              Draw_Main_Menu(3);
          }
          break;
      }
      break;

    #if HAS_MESH
      case Leveling:

        #define LEVELING_BACK 0
        #define LEVELING_ACTIVE (LEVELING_BACK + 1)
        #define LEVELING_GET_TILT (LEVELING_ACTIVE + BOTH(HAS_BED_PROBE, AUTO_BED_LEVELING_UBL))
        #define LEVELING_GET_MESH (LEVELING_GET_TILT + 1)
        #define LEVELING_MANUAL (LEVELING_GET_MESH + 1)
        #define LEVELING_VIEW (LEVELING_MANUAL + 1)
        #define LEVELING_SETTINGS (LEVELING_VIEW + 1)
        #define LEVELING_SLOT (LEVELING_SETTINGS + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_LOAD (LEVELING_SLOT + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_SAVE (LEVELING_LOAD + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_TOTAL LEVELING_SAVE

        switch (item) {
          case LEVELING_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Main_Menu(3);
            break;
          case LEVELING_ACTIVE:
            if (draw) {
              Draw_Menu_Item(row, ICON_StockConfiguration, F("Leveling Active"));
              Draw_Checkbox(row, planner.leveling_active);
            }
            else {
              if (!planner.leveling_active) {
                set_bed_leveling_enabled(!planner.leveling_active);
                if (!planner.leveling_active) {
                  Confirm_Handler(LevelError);
                  break;
                }
              }
              else
                set_bed_leveling_enabled(!planner.leveling_active);
              Draw_Checkbox(row, planner.leveling_active);
            }
            break;
          #if BOTH(HAS_BED_PROBE, AUTO_BED_LEVELING_UBL)
            case LEVELING_GET_TILT:
              if (draw)
                Draw_Menu_Item(row, ICON_Tilt, F("Autotilt Current Mesh"));
              else {
                if (ubl.storage_slot < 0) {
                  Popup_Handler(MeshSlot);
                  break;
                }
                Popup_Handler(Home);
                gcode.home_all_axes(true);
                Popup_Handler(Level);
                if (mesh_conf.tilt_grid > 1) {
                  sprintf_P(cmd, PSTR("G29 J%i"), mesh_conf.tilt_grid);
                  gcode.process_subcommands_now(cmd);
                }
                else
                  gcode.process_subcommands_now(F("G29 J"));
                planner.synchronize();
                Redraw_Menu();
              }
              break;
          #endif
          case LEVELING_GET_MESH:
            if (draw)
              Draw_Menu_Item(row, ICON_Mesh, F("Create New Mesh"));
            else {
              Popup_Handler(Home);
              gcode.home_all_axes(true);
              #if ENABLED(AUTO_BED_LEVELING_UBL)
                #if ENABLED(PREHEAT_BEFORE_LEVELING)
                  Popup_Handler(Heating);
                  #if HAS_HOTEND
                    if (thermalManager.degTargetHotend(0) < LEVELING_NOZZLE_TEMP)
                      thermalManager.setTargetHotend(LEVELING_NOZZLE_TEMP, 0);
                  #endif
                  #if HAS_HEATED_BED
                    if (thermalManager.degTargetBed() < LEVELING_BED_TEMP)
                      thermalManager.setTargetBed(LEVELING_BED_TEMP);
                  #endif
                  thermalManager.wait_for_hotend(0);
                  TERN_(HAS_HEATED_BED, thermalManager.wait_for_bed_heating());
                #endif
                #if HAS_BED_PROBE
                  Popup_Handler(Level);
                  gcode.process_subcommands_now(F("G29 P0\nG29 P1"));
                  gcode.process_subcommands_now(F("G29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nG29 P3\nM420 S1"));
                  planner.synchronize();
                  Update_Status("Probed all reachable points");
                  Popup_Handler(SaveLevel);
                #else
                  level_state = planner.leveling_active;
                  set_bed_leveling_enabled(false);
                  mesh_conf.goto_mesh_value = true;
                  mesh_conf.mesh_x = mesh_conf.mesh_y = 0;
                  Popup_Handler(MoveWait);
                  mesh_conf.manual_move();;
                  Draw_Menu(UBLMesh);
                #endif
              #elif HAS_BED_PROBE
                Popup_Handler(Level);
                gcode.process_subcommands_now(F("G29"));
                planner.synchronize();
                Popup_Handler(SaveLevel);
              #else
                level_state = planner.leveling_active;
                set_bed_leveling_enabled(false);
                gridpoint = 1;
                Popup_Handler(MoveWait);
                gcode.process_subcommands_now(F("G29"));
                planner.synchronize();
                Draw_Menu(ManualMesh);
              #endif
            }
            break;
          case LEVELING_MANUAL:
            if (draw)
              Draw_Menu_Item(row, ICON_Mesh, F("Manual Tuning"), nullptr, true);
            else {
              #if ENABLED(AUTO_BED_LEVELING_BILINEAR)
                if (!leveling_is_valid()) {
                  Confirm_Handler(InvalidMesh);
                  break;
                }
              #endif
              #if ENABLED(AUTO_BED_LEVELING_UBL)
                if (ubl.storage_slot < 0) {
                  Popup_Handler(MeshSlot);
                  break;
                }
              #endif
              if (axes_should_home()) {
                Popup_Handler(Home);
                gcode.home_all_axes(true);
              }
              level_state = planner.leveling_active;
              set_bed_leveling_enabled(false);
              mesh_conf.goto_mesh_value = false;
              #if ENABLED(PREHEAT_BEFORE_LEVELING)
                Popup_Handler(Heating);
                #if HAS_HOTEND
                  if (thermalManager.degTargetHotend(0) < LEVELING_NOZZLE_TEMP)
                    thermalManager.setTargetHotend(LEVELING_NOZZLE_TEMP, 0);
                #endif
                #if HAS_HEATED_BED
                  if (thermalManager.degTargetBed() < LEVELING_BED_TEMP)
                    thermalManager.setTargetBed(LEVELING_BED_TEMP);
                #endif
                TERN_(HAS_HOTEND, thermalManager.wait_for_hotend(0));
                TERN_(HAS_HEATED_BED, thermalManager.wait_for_bed_heating());
              #endif
              Popup_Handler(MoveWait);
              mesh_conf.manual_move();
              Draw_Menu(LevelManual);
            }
            break;
          case LEVELING_VIEW:
            if (draw)
              Draw_Menu_Item(row, ICON_Mesh, GET_TEXT(MSG_MESH_VIEW), nullptr, true);
            else {
              #if ENABLED(AUTO_BED_LEVELING_UBL)
                if (ubl.storage_slot < 0) {
                  Popup_Handler(MeshSlot);
                  break;
                }
              #endif
              Draw_Menu(LevelView);
            }
            break;
          case LEVELING_SETTINGS:
            if (draw)
              Draw_Menu_Item(row, ICON_Step, F("Leveling Settings"), nullptr, true);
            else
              Draw_Menu(LevelSettings);
            break;
          #if ENABLED(AUTO_BED_LEVELING_UBL)
          case LEVELING_SLOT:
            if (draw) {
              Draw_Menu_Item(row, ICON_PrintSize, F("Mesh Slot"));
              Draw_Float(ubl.storage_slot, row, false, 1);
            }
            else
              Modify_Value(ubl.storage_slot, 0, settings.calc_num_meshes() - 1, 1);
            break;
          case LEVELING_LOAD:
            if (draw)
              Draw_Menu_Item(row, ICON_ReadEEPROM, F("Load Mesh"));
            else {
              if (ubl.storage_slot < 0) {
                Popup_Handler(MeshSlot);
                break;
              }
              gcode.process_subcommands_now(F("G29 L"));
              planner.synchronize();
              AudioFeedback(true);
            }
            break;
          case LEVELING_SAVE:
            if (draw)
              Draw_Menu_Item(row, ICON_WriteEEPROM, F("Save Mesh"));
            else {
              if (ubl.storage_slot < 0) {
                Popup_Handler(MeshSlot);
                break;
              }
              gcode.process_subcommands_now(F("G29 S"));
              planner.synchronize();
              AudioFeedback(true);
            }
            break;
          #endif
        }
        break;

      case LevelView:

        #define LEVELING_VIEW_BACK 0
        #define LEVELING_VIEW_MESH (LEVELING_VIEW_BACK + 1)
        #define LEVELING_VIEW_TEXT (LEVELING_VIEW_MESH + 1)
        #define LEVELING_VIEW_ASYMMETRIC (LEVELING_VIEW_TEXT + 1)
        #define LEVELING_VIEW_TOTAL LEVELING_VIEW_ASYMMETRIC

        switch (item) {
          case LEVELING_VIEW_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(Leveling, LEVELING_VIEW);
            break;
          case LEVELING_VIEW_MESH:
            if (draw)
              Draw_Menu_Item(row, ICON_PrintSize, GET_TEXT(MSG_MESH_VIEW), nullptr, true);
            else
              Draw_Menu(MeshViewer);
            break;
          case LEVELING_VIEW_TEXT:
            if (draw) {
              Draw_Menu_Item(row, ICON_Contact, F("Viewer Show Values"));
              Draw_Checkbox(row, mesh_conf.viewer_print_value);
            }
            else {
              mesh_conf.viewer_print_value = !mesh_conf.viewer_print_value;
              Draw_Checkbox(row, mesh_conf.viewer_print_value);
            }
            break;
          case LEVELING_VIEW_ASYMMETRIC:
            if (draw) {
              Draw_Menu_Item(row, ICON_Axis, F("Viewer Asymmetric"));
              Draw_Checkbox(row, mesh_conf.viewer_asymmetric_range);
            }
            else {
              mesh_conf.viewer_asymmetric_range = !mesh_conf.viewer_asymmetric_range;
              Draw_Checkbox(row, mesh_conf.viewer_asymmetric_range);
            }
            break;
        }
        break;

      case LevelSettings:

        #define LEVELING_SETTINGS_BACK 0
        #define LEVELING_SETTINGS_FADE (LEVELING_SETTINGS_BACK + 1)
        #define LEVELING_SETTINGS_TILT (LEVELING_SETTINGS_FADE + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_SETTINGS_PLANE (LEVELING_SETTINGS_TILT + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_SETTINGS_ZERO (LEVELING_SETTINGS_PLANE + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_SETTINGS_UNDEF (LEVELING_SETTINGS_ZERO + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_SETTINGS_TOTAL LEVELING_SETTINGS_UNDEF

        switch (item) {
          case LEVELING_SETTINGS_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else
              Draw_Menu(Leveling, LEVELING_SETTINGS);
            break;
          case LEVELING_SETTINGS_FADE:
              if (draw) {
                Draw_Menu_Item(row, ICON_Fade, F("Fade Mesh within"));
                Draw_Float(planner.z_fade_height, row, false, 1);
              }
              else {
                Modify_Value(planner.z_fade_height, 0, Z_MAX_POS, 1);
                planner.z_fade_height = -1;
                set_z_fade_height(planner.z_fade_height);
              }
              break;

          #if ENABLED(AUTO_BED_LEVELING_UBL)
            case LEVELING_SETTINGS_TILT:
              if (draw) {
                Draw_Menu_Item(row, ICON_Tilt, F("Tilting Grid Size"));
                Draw_Float(mesh_conf.tilt_grid, row, false, 1);
              }
              else
                Modify_Value(mesh_conf.tilt_grid, 1, 8, 1);
              break;
            case LEVELING_SETTINGS_PLANE:
              if (draw)
                Draw_Menu_Item(row, ICON_ResumeEEPROM, F("Convert Mesh to Plane"));
              else {
                if (mesh_conf.create_plane_from_mesh()) break;
                gcode.process_subcommands_now(F("M420 S1"));
                planner.synchronize();
                AudioFeedback(true);
              }
              break;
            case LEVELING_SETTINGS_ZERO:
              if (draw)
                Draw_Menu_Item(row, ICON_Mesh, F("Zero Current Mesh"));
              else
                ZERO(Z_VALUES_ARR);
              break;
            case LEVELING_SETTINGS_UNDEF:
              if (draw)
                Draw_Menu_Item(row, ICON_Mesh, F("Clear Current Mesh"));
              else
                ubl.invalidate();
              break;
          #endif // AUTO_BED_LEVELING_UBL
        }
        break;

      case MeshViewer:
        #define MESHVIEW_BACK 0
        #define MESHVIEW_TOTAL MESHVIEW_BACK

        if (item == MESHVIEW_BACK) {
          if (draw) {
            Draw_Menu_Item(0, ICON_Back, F("Back"));
            mesh_conf.Draw_Bed_Mesh();
            mesh_conf.Set_Mesh_Viewer_Status();
          }
          else if (!mesh_conf.drawing_mesh) {
            Draw_Menu(LevelView, LEVELING_VIEW_MESH);
            Update_Status("");
          }
        }
        break;

      case LevelManual:

        #define LEVELING_M_BACK 0
        #define LEVELING_M_X (LEVELING_M_BACK + 1)
        #define LEVELING_M_Y (LEVELING_M_X + 1)
        #define LEVELING_M_NEXT (LEVELING_M_Y + 1)
        #define LEVELING_M_OFFSET (LEVELING_M_NEXT + 1)
        #define LEVELING_M_UP (LEVELING_M_OFFSET + 1)
        #define LEVELING_M_DOWN (LEVELING_M_UP + 1)
        #define LEVELING_M_GOTO_VALUE (LEVELING_M_DOWN + 1)
        #define LEVELING_M_UNDEF (LEVELING_M_GOTO_VALUE + ENABLED(AUTO_BED_LEVELING_UBL))
        #define LEVELING_M_TOTAL LEVELING_M_UNDEF

        switch (item) {
          case LEVELING_M_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else {
              set_bed_leveling_enabled(level_state);
              TERN_(AUTO_BED_LEVELING_BILINEAR, refresh_bed_level());
              Draw_Menu(Leveling, LEVELING_MANUAL);
            }
            break;
          case LEVELING_M_X:
            if (draw) {
              Draw_Menu_Item(row, ICON_MoveX, F("Mesh Point X"));
              Draw_Float(mesh_conf.mesh_x, row, 0, 1);
            }
            else
              Modify_Value(mesh_conf.mesh_x, 0, GRID_MAX_POINTS_X - 1, 1);
            break;
          case LEVELING_M_Y:
            if (draw) {
              Draw_Menu_Item(row, ICON_MoveY, F("Mesh Point Y"));
              Draw_Float(mesh_conf.mesh_y, row, 0, 1);
            }
            else
              Modify_Value(mesh_conf.mesh_y, 0, GRID_MAX_POINTS_Y - 1, 1);
            break;
          case LEVELING_M_NEXT:
            if (draw)
              Draw_Menu_Item(row, ICON_More, F("Next Point"));
            else {
              if (mesh_conf.mesh_x != (GRID_MAX_POINTS_X - 1) || mesh_conf.mesh_y != (GRID_MAX_POINTS_Y - 1)) {
                if ((mesh_conf.mesh_x == (GRID_MAX_POINTS_X - 1) && mesh_conf.mesh_y % 2 == 0) || (mesh_conf.mesh_x == 0 && mesh_conf.mesh_y % 2 == 1))
                  mesh_conf.mesh_y++;
                else if (mesh_conf.mesh_y % 2 == 0)
                  mesh_conf.mesh_x++;
                else
                  mesh_conf.mesh_x--;
                mesh_conf.manual_move();
              }
            }
            break;
          case LEVELING_M_OFFSET:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetZOffset, F("Point Z Offset"));
              Draw_Float(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], row, false, 100);
            }
            else {
              if (isnan(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y]))
                Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] = 0;
              Modify_Value(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], MIN_Z_OFFSET, MAX_Z_OFFSET, 100);
            }
            break;
          case LEVELING_M_UP:
            if (draw)
              Draw_Menu_Item(row, ICON_Axis, F("Microstep Up"));
            else if (Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] < MAX_Z_OFFSET) {
              Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] += 0.01;
              gcode.process_subcommands_now(F("M290 Z0.01"));
              planner.synchronize();
              current_position.z += 0.01f;
              sync_plan_position();
              Draw_Float(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], row - 1, false, 100);
            }
            break;
          case LEVELING_M_DOWN:
            if (draw)
              Draw_Menu_Item(row, ICON_AxisD, F("Microstep Down"));
            else if (Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] > MIN_Z_OFFSET) {
              Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] -= 0.01;
              gcode.process_subcommands_now(F("M290 Z-0.01"));
              planner.synchronize();
              current_position.z -= 0.01f;
              sync_plan_position();
              Draw_Float(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], row - 2, false, 100);
            }
            break;
          case LEVELING_M_GOTO_VALUE:
            if (draw) {
              Draw_Menu_Item(row, ICON_StockConfiguration, F("Go to Mesh Z Value"));
              Draw_Checkbox(row, mesh_conf.goto_mesh_value);
            }
            else {
              mesh_conf.goto_mesh_value = !mesh_conf.goto_mesh_value;
              current_position.z = 0;
              mesh_conf.manual_move(true);
              Draw_Checkbox(row, mesh_conf.goto_mesh_value);
            }
            break;
          #if ENABLED(AUTO_BED_LEVELING_UBL)
          case LEVELING_M_UNDEF:
            if (draw)
              Draw_Menu_Item(row, ICON_ResumeEEPROM, F("Clear Point Value"));
            else {
              mesh_conf.manual_value_update(true);
              Redraw_Menu(false);
            }
            break;
          #endif
        }
        break;
    #endif // HAS_MESH

    #if ENABLED(AUTO_BED_LEVELING_UBL) && !HAS_BED_PROBE
      case UBLMesh:

        #define UBL_M_BACK 0
        #define UBL_M_NEXT (UBL_M_BACK + 1)
        #define UBL_M_PREV (UBL_M_NEXT + 1)
        #define UBL_M_OFFSET (UBL_M_PREV + 1)
        #define UBL_M_UP (UBL_M_OFFSET + 1)
        #define UBL_M_DOWN (UBL_M_UP + 1)
        #define UBL_M_TOTAL UBL_M_DOWN

        switch (item) {
          case UBL_M_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Back"));
            else {
              set_bed_leveling_enabled(level_state);
              Draw_Menu(Leveling, LEVELING_GET_MESH);
            }
            break;
          case UBL_M_NEXT:
            if (draw) {
              if (mesh_conf.mesh_x != (GRID_MAX_POINTS_X - 1) || mesh_conf.mesh_y != (GRID_MAX_POINTS_Y - 1))
                Draw_Menu_Item(row, ICON_More, F("Next Point"));
              else
                Draw_Menu_Item(row, ICON_More, F("Save Mesh"));
            }
            else {
              if (mesh_conf.mesh_x != (GRID_MAX_POINTS_X - 1) || mesh_conf.mesh_y != (GRID_MAX_POINTS_Y - 1)) {
                if ((mesh_conf.mesh_x == (GRID_MAX_POINTS_X - 1) && mesh_conf.mesh_y % 2 == 0) || (mesh_conf.mesh_x == 0 && mesh_conf.mesh_y % 2 == 1))
                  mesh_conf.mesh_y++;
                else if (mesh_conf.mesh_y % 2 == 0)
                  mesh_conf.mesh_x++;
                else
                  mesh_conf.mesh_x--;
                mesh_conf.manual_move();
              }
              else {
                gcode.process_subcommands_now(F("G29 S"));
                planner.synchronize();
                AudioFeedback(true);
                Draw_Menu(Leveling, LEVELING_GET_MESH);
              }
            }
            break;
          case UBL_M_PREV:
            if (draw)
              Draw_Menu_Item(row, ICON_More, F("Previous Point"));
            else {
              if (mesh_conf.mesh_x != 0 || mesh_conf.mesh_y != 0) {
                if ((mesh_conf.mesh_x == (GRID_MAX_POINTS_X - 1) && mesh_conf.mesh_y % 2 == 1) || (mesh_conf.mesh_x == 0 && mesh_conf.mesh_y % 2 == 0))
                  mesh_conf.mesh_y--;
                else if (mesh_conf.mesh_y % 2 == 0)
                  mesh_conf.mesh_x--;
                else
                  mesh_conf.mesh_x++;
                mesh_conf.manual_move();
              }
            }
            break;
          case UBL_M_OFFSET:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetZOffset, F("Point Z Offset"));
              Draw_Float(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], row, false, 100);
            }
            else {
              if (isnan(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y]))
                Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] = 0;
              Modify_Value(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], MIN_Z_OFFSET, MAX_Z_OFFSET, 100);
            }
            break;
          case UBL_M_UP:
            if (draw)
              Draw_Menu_Item(row, ICON_Axis, F("Microstep Up"));
            else if (Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] < MAX_Z_OFFSET) {
              Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] += 0.01;
              gcode.process_subcommands_now(F("M290 Z0.01"));
              planner.synchronize();
              current_position.z += 0.01f;
              sync_plan_position();
              Draw_Float(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], row - 1, false, 100);
            }
            break;
          case UBL_M_DOWN:
            if (draw)
              Draw_Menu_Item(row, ICON_Axis, F("Microstep Down"));
            else if (Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] > MIN_Z_OFFSET) {
              Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y] -= 0.01;
              gcode.process_subcommands_now(F("M290 Z-0.01"));
              planner.synchronize();
              current_position.z -= 0.01f;
              sync_plan_position();
              Draw_Float(Z_VALUES_ARR[mesh_conf.mesh_x][mesh_conf.mesh_y], row - 2, false, 100);
            }
            break;
        }
        break;
    #endif // AUTO_BED_LEVELING_UBL && !HAS_BED_PROBE

    #if ENABLED(PROBE_MANUALLY)
      case ManualMesh:

        #define MMESH_BACK 0
        #define MMESH_NEXT (MMESH_BACK + 1)
        #define MMESH_OFFSET (MMESH_NEXT + 1)
        #define MMESH_UP (MMESH_OFFSET + 1)
        #define MMESH_DOWN (MMESH_UP + 1)
        #define MMESH_OLD (MMESH_DOWN + 1)
        #define MMESH_TOTAL MMESH_OLD

        switch (item) {
          case MMESH_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Cancel"));
            else {
              gcode.process_subcommands_now(F("G29 A"));
              planner.synchronize();
              set_bed_leveling_enabled(level_state);
              Draw_Menu(Leveling, LEVELING_GET_MESH);
            }
            break;
          case MMESH_NEXT:
            if (draw) {
              if (gridpoint < GRID_MAX_POINTS)
                Draw_Menu_Item(row, ICON_More, F("Next Point"));
              else
                Draw_Menu_Item(row, ICON_More, F("Save Mesh"));
            }
            else if (gridpoint < GRID_MAX_POINTS) {
              Popup_Handler(MoveWait);
              gcode.process_subcommands_now(F("G29"));
              planner.synchronize();
              gridpoint++;
              Redraw_Menu();
            }
            else {
              gcode.process_subcommands_now(F("G29"));
              planner.synchronize();
              AudioFeedback(settings.save());
              Draw_Menu(Leveling, LEVELING_GET_MESH);
            }
            break;
          case MMESH_OFFSET:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetZOffset, F("Z Position"));
              current_position.z = MANUAL_PROBE_START_Z;
              Draw_Float(current_position.z, row, false, 100);
            }
            else
              Modify_Value(current_position.z, MIN_Z_OFFSET, MAX_Z_OFFSET, 100);
            break;
          case MMESH_UP:
            if (draw)
              Draw_Menu_Item(row, ICON_Axis, F("Microstep Up"));
            else if (current_position.z < MAX_Z_OFFSET) {
              gcode.process_subcommands_now(F("M290 Z0.01"));
              planner.synchronize();
              current_position.z += 0.01f;
              sync_plan_position();
              Draw_Float(current_position.z, row - 1, false, 100);
            }
            break;
          case MMESH_DOWN:
            if (draw)
              Draw_Menu_Item(row, ICON_AxisD, F("Microstep Down"));
            else if (current_position.z > MIN_Z_OFFSET) {
              gcode.process_subcommands_now(F("M290 Z-0.01"));
              planner.synchronize();
              current_position.z -= 0.01f;
              sync_plan_position();
              Draw_Float(current_position.z, row - 2, false, 100);
            }
            break;
          case MMESH_OLD:
            uint8_t mesh_x, mesh_y;
            // 0,0 -> 1,0 -> 2,0 -> 2,1 -> 1,1 -> 0,1 -> 0,2 -> 1,2 -> 2,2
            mesh_y = (gridpoint - 1) / GRID_MAX_POINTS_Y;
            mesh_x = (gridpoint - 1) % GRID_MAX_POINTS_X;

            if (mesh_y % 2 == 1)
              mesh_x = GRID_MAX_POINTS_X - mesh_x - 1;

            const float currval = Z_VALUES_ARR[mesh_x][mesh_y];

            if (draw) {
              Draw_Menu_Item(row, ICON_Zoffset, F("Goto Mesh Value"));
              Draw_Float(currval, row, false, 100);
            }
            else if (!isnan(currval)) {
              current_position.z = currval;
              planner.synchronize();
              planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
              planner.synchronize();
              Draw_Float(current_position.z, row - 3, false, 100);
            }
            break;
        }
        break;
    #endif // PROBE_MANUALLY

    case Tune:

      #define TUNE_BACK 0
      #define TUNE_SPEED (TUNE_BACK + 1)
      #define TUNE_FLOW (TUNE_SPEED + ENABLED(HAS_HOTEND))
      #define TUNE_HOTEND (TUNE_FLOW + ENABLED(HAS_HOTEND))
      #define TUNE_BED (TUNE_HOTEND + ENABLED(HAS_HEATED_BED))
      #define TUNE_FAN (TUNE_BED + ENABLED(HAS_FAN))
      #define TUNE_ZOFFSET (TUNE_FAN + ENABLED(HAS_ZOFFSET_ITEM))
      #define TUNE_ZUP (TUNE_ZOFFSET + ENABLED(HAS_ZOFFSET_ITEM))
      #define TUNE_ZDOWN (TUNE_ZUP + ENABLED(HAS_ZOFFSET_ITEM))
      #define TUNE_CHANGEFIL (TUNE_ZDOWN + ENABLED(FILAMENT_LOAD_UNLOAD_GCODES))
      #define TUNE_FILSENSORENABLED (TUNE_CHANGEFIL + ENABLED(FILAMENT_RUNOUT_SENSOR))
      #define TUNE_BACKLIGHT_OFF (TUNE_FILSENSORENABLED + 1)
      #define TUNE_BACKLIGHT (TUNE_BACKLIGHT_OFF + 1)
      #define TUNE_TOTAL TUNE_BACKLIGHT

      switch (item) {
        case TUNE_BACK:
          if (draw)
            Draw_Menu_Item(row, ICON_Back, F("Back"));
          else
            Draw_Print_Screen();
          break;
        case TUNE_SPEED:
          if (draw) {
            Draw_Menu_Item(row, ICON_Speed, F("Print Speed"));
            Draw_Float(feedrate_percentage, row, false, 1);
          }
          else
            Modify_Value(feedrate_percentage, MIN_PRINT_SPEED, MAX_PRINT_SPEED, 1);
          break;

        #if HAS_HOTEND
          case TUNE_FLOW:
            if (draw) {
              Draw_Menu_Item(row, ICON_Speed, F("Flow Rate"));
              Draw_Float(planner.flow_percentage[0], row, false, 1);
            }
            else
              Modify_Value(planner.flow_percentage[0], MIN_FLOW_RATE, MAX_FLOW_RATE, 1);
            break;
          case TUNE_HOTEND:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetEndTemp, F("Hotend"));
              Draw_Float(thermalManager.temp_hotend[0].target, row, false, 1);
            }
            else
              Modify_Value(thermalManager.temp_hotend[0].target, MIN_E_TEMP, MAX_E_TEMP, 1);
            break;
        #endif

        #if HAS_HEATED_BED
          case TUNE_BED:
            if (draw) {
              Draw_Menu_Item(row, ICON_SetBedTemp, F("Bed"));
              Draw_Float(thermalManager.temp_bed.target, row, false, 1);
            }
            else
              Modify_Value(thermalManager.temp_bed.target, MIN_BED_TEMP, MAX_BED_TEMP, 1);
            break;
        #endif

        #if HAS_FAN
          case TUNE_FAN:
            if (draw) {
              Draw_Menu_Item(row, ICON_FanSpeed, F("Fan"));
              Draw_Float(thermalManager.fan_speed[0], row, false, 1);
            }
            else
              Modify_Value(thermalManager.fan_speed[0], MIN_FAN_SPEED, MAX_FAN_SPEED, 1);
            break;
        #endif

        #if HAS_ZOFFSET_ITEM
          case TUNE_ZOFFSET:
            if (draw) {
              Draw_Menu_Item(row, ICON_FanSpeed, F("Z-Offset"));
              Draw_Float(zoffsetvalue, row, false, 100);
            }
            else
              Modify_Value(zoffsetvalue, MIN_Z_OFFSET, MAX_Z_OFFSET, 100);
            break;
          case TUNE_ZUP:
            if (draw)
              Draw_Menu_Item(row, ICON_Axis, F("Z-Offset Up"));
            else if (zoffsetvalue < MAX_Z_OFFSET) {
              gcode.process_subcommands_now(F("M290 Z0.01"));
              zoffsetvalue += 0.01;
              Draw_Float(zoffsetvalue, row - 1, false, 100);
            }
            break;
          case TUNE_ZDOWN:
            if (draw)
              Draw_Menu_Item(row, ICON_AxisD, F("Z-Offset Down"));
            else if (zoffsetvalue > MIN_Z_OFFSET) {
              gcode.process_subcommands_now(F("M290 Z-0.01"));
              zoffsetvalue -= 0.01;
              Draw_Float(zoffsetvalue, row - 2, false, 100);
            }
            break;
        #endif

        #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
          case TUNE_CHANGEFIL:
            if (draw)
              Draw_Menu_Item(row, ICON_ResumeEEPROM, F("Change Filament"));
            else
              Popup_Handler(ConfFilChange);
            break;
        #endif

        #if ENABLED(FILAMENT_RUNOUT_SENSOR)
          case TUNE_FILSENSORENABLED:
            if (draw) {
              Draw_Menu_Item(row, ICON_Extruder, F("Filament Sensor"));
              Draw_Checkbox(row, runout.enabled);
            }
            else {
              runout.enabled = !runout.enabled;
              Draw_Checkbox(row, runout.enabled);
            }
            break;
        #endif

        case TUNE_BACKLIGHT_OFF:
          if (draw)
            Draw_Menu_Item(row, ICON_Brightness, F("Display Off"));
          else
            ui.set_brightness(0);
          break;
        case TUNE_BACKLIGHT:
          if (draw) {
            Draw_Menu_Item(row, ICON_Brightness, F("LCD Brightness"));
            Draw_Float(ui.brightness, row, false, 1);
          }
          else
            Modify_Value(ui.brightness, LCD_BRIGHTNESS_MIN, LCD_BRIGHTNESS_MAX, 1, ui.refresh_brightness);
          break;
      }
      break;

    case PreheatHotend:

        #define PREHEATHOTEND_BACK 0
        #define PREHEATHOTEND_CONTINUE (PREHEATHOTEND_BACK + 1)
        #define PREHEATHOTEND_1 (PREHEATHOTEND_CONTINUE + (PREHEAT_COUNT >= 1))
        #define PREHEATHOTEND_2 (PREHEATHOTEND_1 + (PREHEAT_COUNT >= 2))
        #define PREHEATHOTEND_3 (PREHEATHOTEND_2 + (PREHEAT_COUNT >= 3))
        #define PREHEATHOTEND_4 (PREHEATHOTEND_3 + (PREHEAT_COUNT >= 4))
        #define PREHEATHOTEND_5 (PREHEATHOTEND_4 + (PREHEAT_COUNT >= 5))
        #define PREHEATHOTEND_CUSTOM (PREHEATHOTEND_5 + 1)
        #define PREHEATHOTEND_TOTAL PREHEATHOTEND_CUSTOM

        switch (item) {
          case PREHEATHOTEND_BACK:
            if (draw)
              Draw_Menu_Item(row, ICON_Back, F("Cancel"));
            else {
              thermalManager.setTargetHotend(0, 0);
              thermalManager.set_fan_speed(0, 0);
              Redraw_Menu(false, true, true);
            }
            break;
          case PREHEATHOTEND_CONTINUE:
            if (draw)
              Draw_Menu_Item(row, ICON_SetEndTemp, F("Continue"));
            else {
              Popup_Handler(Heating);
              thermalManager.wait_for_hotend(0);
              switch (last_menu) {
                case Prepare:
                  Popup_Handler(FilChange);
                  sprintf_P(cmd, PSTR("M600 B1 R%i"), thermalManager.temp_hotend[0].target);
                  gcode.process_subcommands_now(cmd);
                  break;
                #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
                  case ChangeFilament:
                    switch (last_selection) {
                      case CHANGEFIL_LOAD:
                        Popup_Handler(FilLoad);
                        gcode.process_subcommands_now(F("M701"));
                        planner.synchronize();
                        Redraw_Menu(true, true, true);
                        break;
                      case CHANGEFIL_UNLOAD:
                        Popup_Handler(FilLoad, true);
                        gcode.process_subcommands_now(F("M702"));
                        planner.synchronize();
                        Redraw_Menu(true, true, true);
                        break;
                      case CHANGEFIL_CHANGE:
                        Popup_Handler(FilChange);
                        sprintf_P(cmd, PSTR("M600 B1 R%i"), thermalManager.temp_hotend[0].target);
                        gcode.process_subcommands_now(cmd);
                        break;
                    }
                    break;
                #endif
                default:
                  Redraw_Menu(true, true, true);
                  break;
              }
            }
            break;
          #if PREHEAT_COUNT >= 1
            case PREHEATHOTEND_1:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_1_LABEL));
              else
                ui.preheat_hotend_and_fan(0);
              break;
          #endif
          #if PREHEAT_COUNT >= 2
            case PREHEATHOTEND_2:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_2_LABEL));
              else
                ui.preheat_hotend_and_fan(1);
              break;
          #endif
          #if PREHEAT_COUNT >= 3
            case PREHEATHOTEND_3:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_3_LABEL));
              else
                ui.preheat_hotend_and_fan(2);
              break;
          #endif
          #if PREHEAT_COUNT >= 4
            case PREHEATHOTEND_4:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_4_LABEL));
              else
                ui.preheat_hotend_and_fan(3);
              break;
          #endif
          #if PREHEAT_COUNT >= 5
            case PREHEATHOTEND_5:
              if (draw)
                Draw_Menu_Item(row, ICON_Temperature, F(PREHEAT_5_LABEL));
              else
                ui.preheat_hotend_and_fan(4);
              break;
          #endif
          case PREHEATHOTEND_CUSTOM:
            if (draw) {
              Draw_Menu_Item(row, ICON_Temperature, F("Custom"));
              Draw_Float(thermalManager.temp_hotend[0].target, row, false, 1);
            }
            else
              Modify_Value(thermalManager.temp_hotend[0].target, EXTRUDE_MINTEMP, MAX_E_TEMP, 1);
            break;
        }
        break;
  }
}

FSTR_P CrealityDWINClass::Get_Menu_Title(uint8_t menu) {
  switch (menu) {
    case MainMenu:          return F("Main Menu");
    case Prepare:           return F("Prepare");
    case HomeMenu:          return F("Homing Menu");
    case Move:              return F("Move");
    case ManualLevel:       return F("Manual Leveling");
    #if HAS_ZOFFSET_ITEM
      case ZOffset:         return F("Z Offset");
    #endif
    #if HAS_PREHEAT
      case Preheat:         return F("Preheat");
    #endif
    #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
      case ChangeFilament:  return F("Change Filament");
    #endif
    #if ENABLED(HOST_ACTION_COMMANDS)
      case HostActions:       return F("Host Action Commands");
    #endif
    case Control:           return F("Control");
    case TempMenu:          return F("Temperature");
    #if HAS_HOTEND || HAS_HEATED_BED
      case PID:             return F("PID Menu");
    #endif
    #if HAS_HOTEND
      case HotendPID:       return F("Hotend PID Settings");
    #endif
    #if HAS_HEATED_BED
      case BedPID:          return F("Bed PID Settings");
    #endif
    #if PREHEAT_COUNT >= 1
      case Preheat1:        return F(PREHEAT_1_LABEL " Settings");
    #endif
    #if PREHEAT_COUNT >= 2
      case Preheat2:        return F(PREHEAT_2_LABEL " Settings");
    #endif
    #if PREHEAT_COUNT >= 3
      case Preheat3:        return F(PREHEAT_3_LABEL " Settings");
    #endif
    #if PREHEAT_COUNT >= 4
      case Preheat4:        return F(PREHEAT_4_LABEL " Settings");
    #endif
    #if PREHEAT_COUNT >= 5
      case Preheat5:        return F(PREHEAT_5_LABEL " Settings");
    #endif
    case Motion:            return F("Motion Settings");
    case HomeOffsets:       return F("Home Offsets");
    case MaxSpeed:          return F("Max Speed");
    case MaxAcceleration:   return F("Max Acceleration");
    #if HAS_CLASSIC_JERK
      case MaxJerk:         return F("Max Jerk");
    #endif
    case Steps:             return F("Steps/mm");
    case Visual:            return F("Visual Settings");
    case HostSettings:      return F("Host Settings");
    #if ENABLED(HOST_ACTION_COMMANDS)
      case ActionCommands:    return F("Host Action Settings");
    #endif
    case Advanced:          return F("Advanced Settings");
    #if HAS_BED_PROBE
      case ProbeMenu:       return F("Probe Menu");
    #endif
    case ColorSettings:     return F("UI Color Settings");
    case Info:              return F("Info");
    case InfoMain:          return F("Info");
    #if HAS_MESH
      case Leveling:        return F("Leveling");
      case LevelView:       return GET_TEXT_F(MSG_MESH_VIEW);
      case LevelSettings:   return F("Leveling Settings");
      case MeshViewer:      return GET_TEXT_F(MSG_MESH_VIEW);
      case LevelManual:     return F("Manual Tuning");
    #endif
    #if ENABLED(AUTO_BED_LEVELING_UBL) && !HAS_BED_PROBE
      case UBLMesh:         return F("UBL Bed Leveling");
    #endif
    #if ENABLED(PROBE_MANUALLY)
      case ManualMesh:      return F("Mesh Bed Leveling");
    #endif
    case Tune:              return F("Tune");
    case PreheatHotend:     return F("Preheat Hotend");
  }
  return F("");
}

uint8_t CrealityDWINClass::Get_Menu_Size(uint8_t menu) {
  switch (menu) {
    case Prepare:           return PREPARE_TOTAL;
    case HomeMenu:          return HOME_TOTAL;
    case Move:              return MOVE_TOTAL;
    case ManualLevel:       return MLEVEL_TOTAL;
    #if HAS_ZOFFSET_ITEM
      case ZOffset:         return ZOFFSET_TOTAL;
    #endif
    #if HAS_PREHEAT
      case Preheat:         return PREHEAT_TOTAL;
    #endif
    #if ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)
      case ChangeFilament:  return CHANGEFIL_TOTAL;
    #endif
    #if ENABLED(HOST_ACTION_COMMANDS)
      case HostActions:       return HOSTACTIONS_TOTAL;
    #endif
    case Control:           return CONTROL_TOTAL;
    case TempMenu:          return TEMP_TOTAL;
    #if HAS_HOTEND || HAS_HEATED_BED
      case PID:             return PID_TOTAL;
    #endif
    #if HAS_HOTEND
      case HotendPID:       return HOTENDPID_TOTAL;
    #endif
    #if HAS_HEATED_BED
      case BedPID:          return BEDPID_TOTAL;
    #endif
    #if PREHEAT_COUNT >= 1
      case Preheat1:        return PREHEAT1_TOTAL;
    #endif
    #if PREHEAT_COUNT >= 2
      case Preheat2:        return PREHEAT2_TOTAL;
    #endif
    #if PREHEAT_COUNT >= 3
      case Preheat3:        return PREHEAT3_TOTAL;
    #endif
    #if PREHEAT_COUNT >= 4
      case Preheat4:        return PREHEAT4_TOTAL;
    #endif
    #if PREHEAT_COUNT >= 5
      case Preheat5:        return PREHEAT5_TOTAL;
    #endif
    case Motion:            return MOTION_TOTAL;
    case HomeOffsets:       return HOMEOFFSETS_TOTAL;
    case MaxSpeed:          return SPEED_TOTAL;
    case MaxAcceleration:   return ACCEL_TOTAL;
    #if HAS_CLASSIC_JERK
      case MaxJerk:         return JERK_TOTAL;
    #endif
    case Steps:             return STEPS_TOTAL;
    case Visual:            return VISUAL_TOTAL;
    case HostSettings:      return HOSTSETTINGS_TOTAL;
    #if ENABLED(HOST_ACTION_COMMANDS)
      case ActionCommands:    return ACTIONCOMMANDS_TOTAL;
    #endif
    case Advanced:          return ADVANCED_TOTAL;
    #if HAS_BED_PROBE
      case ProbeMenu:       return PROBE_TOTAL;
    #endif
    case Info:              return INFO_TOTAL;
    case InfoMain:          return INFO_TOTAL;
    #if ENABLED(AUTO_BED_LEVELING_UBL) && !HAS_BED_PROBE
      case UBLMesh:         return UBL_M_TOTAL;
    #endif
    #if ENABLED(PROBE_MANUALLY)
      case ManualMesh:      return MMESH_TOTAL;
    #endif
    #if HAS_MESH
      case Leveling:        return LEVELING_TOTAL;
      case LevelView:       return LEVELING_VIEW_TOTAL;
      case LevelSettings:   return LEVELING_SETTINGS_TOTAL;
      case MeshViewer:      return MESHVIEW_TOTAL;
      case LevelManual:     return LEVELING_M_TOTAL;
    #endif
    case Tune:              return TUNE_TOTAL;
    case PreheatHotend:     return PREHEATHOTEND_TOTAL;
    case ColorSettings:     return COLORSETTINGS_TOTAL;
  }
  return 0;
}
